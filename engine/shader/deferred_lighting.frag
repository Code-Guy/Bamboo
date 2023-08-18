#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput normal_texture_sampler;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput base_color_texture_sampler;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput emissive_color_texture_sampler;
layout(input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput metallic_roughness_occlusion_texture_sampler;
layout(input_attachment_index = 4, set = 0, binding = 4) uniform subpassInput depth_stencil_texture_sampler;

layout(set = 0, binding = 5) uniform samplerCube irradiance_texture_sampler;
layout(set = 0, binding = 6) uniform samplerCube prefilter_texture_sampler;
layout(set = 0, binding = 7) uniform sampler2D brdf_lut_texture_sampler;

layout(set = 0, binding = 8) uniform _LightingUBO { LightingUBO lighting_ubo; };

layout(location = 0) in vec2 f_tex_coord;
layout(location = 0) out vec4 o_color;

struct PBRInfo
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptual_roughness;   // roughness value, as authored by the model creator (input to shader)
	float alpha_roughness;        // roughness mapped to a more linear change in the roughness (proposed by [2])
	float metallic;               // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	vec3 diffuse_color;           // color contribution from diffuse lighting
	vec3 specular_color;          // color contribution from specular lighting
};

vec3 Uncharted2Tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 tonemap(vec3 color)
{
	vec3 uncharted_color = Uncharted2Tonemap(color * lighting_ubo.sky_light.exposure);
	uncharted_color = uncharted_color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	return pow(uncharted_color, vec3(1.0f / STD_GAMMA));
}

vec3 SRGBtoLINEAR(vec3 srgb)
{
	return pow(srgb, vec3(STD_GAMMA));
}

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
vec3 getIBLContribution(PBRInfo pbr_info, vec3 n, vec3 r)
{
	float lod = pbr_info.perceptual_roughness * lighting_ubo.sky_light.prefilter_mip_levels;

	// retrieve a scale and bias to F0. See [1], Figure 3
	vec3 brdf = (texture(brdf_lut_texture_sampler, vec2(pbr_info.NdotV, 1.0 - pbr_info.perceptual_roughness))).rgb;
	vec3 diffuse_light = SRGBtoLINEAR(tonemap(texture(irradiance_texture_sampler, n).rgb));
	vec3 specular_light = SRGBtoLINEAR(tonemap(textureLod(prefilter_texture_sampler, r, lod).rgb));

	vec3 diffuse = diffuse_light * pbr_info.diffuse_color;
	vec3 specular = specular_light * (pbr_info.specular_color * brdf.x + brdf.y);

	return (diffuse) * lighting_ubo.sky_light.color;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(PBRInfo pbr_info)
{
	return pbr_info.diffuse_color / PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbr_info)
{
	return pbr_info.reflectance0 + (pbr_info.reflectance90 - pbr_info.reflectance0) * pow(clamp(1.0 - pbr_info.VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alpha_roughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbr_info)
{
	float NdotL = pbr_info.NdotL;
	float NdotV = pbr_info.NdotV;
	float r = pbr_info.alpha_roughness;

	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbr_info)
{
	float roughnessSq = pbr_info.alpha_roughness * pbr_info.alpha_roughness;
	float f = (pbr_info.NdotH * roughnessSq - pbr_info.NdotH) * pbr_info.NdotH + 1.0;
	return roughnessSq / (PI * f * f);
}

bool is_nearly_equal(float a, float b)
{
    return abs(a - b) < EPSILON;
}

void main()
{
	float depth = subpassLoad(depth_stencil_texture_sampler).x;
	if (is_nearly_equal(depth, 1.0))
	{
		discard;
	}

	// reconstruct position from depth image
    vec4 ndc_pos = vec4(f_tex_coord * 2.0 - 1.0, depth, 1.0);
    vec4 world_position = lighting_ubo.inv_view_proj * ndc_pos;
	vec3 position = world_position.xyz / world_position.w;

	// pbr material properties
	vec4 base_color = subpassLoad(base_color_texture_sampler);
	vec3 emissive_color = subpassLoad(emissive_color_texture_sampler).xyz;
	vec3 metallic_roughness_occlusion = subpassLoad(metallic_roughness_occlusion_texture_sampler).xyz;
	float metallic = metallic_roughness_occlusion.x;
	float perceptual_roughness = metallic_roughness_occlusion.y;
	float alpha_roughness = perceptual_roughness * perceptual_roughness;
	float occlusion = metallic_roughness_occlusion.z;

	// calculate diffuse_color and specular_color
	vec3 f0 = vec3(DIELECTRIC_F0);
	vec3 diffuse_color = base_color.rgb * (vec3(1.0) - f0);
	diffuse_color *= 1.0 - metallic;
	vec3 specular_color = mix(f0, base_color.rgb, metallic);
	
	// calculate reflectance
	float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);

	// for typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// for very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	vec3 specularEnvironmentR0 = specular_color.rgb;
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR90 = reflectance90 * vec3(1.0);

	// surface/camera/light directions
	vec3 n = subpassLoad(normal_texture_sampler).xyz;
	vec3 v = normalize(lighting_ubo.camera_pos - position);
	vec3 l = normalize(-lighting_ubo.directional_light.direction);
	vec3 h = normalize(l + v);
	vec3 r = -normalize(reflect(v, n));

	// surface/camera/light cosine angles
	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float VdotH = clamp(dot(v, h), 0.0, 1.0);

	// construct pbr info structure
	PBRInfo pbr_info = PBRInfo(
		NdotL,
		NdotV,
		NdotH,
		VdotH,
		perceptual_roughness,
		alpha_roughness,
		metallic,
		specularEnvironmentR0,
		specularEnvironmentR90,
		diffuse_color,
		specular_color
	);

	// calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbr_info);
	float G = geometricOcclusion(pbr_info);
	float D = microfacetDistribution(pbr_info);

	// calculation of analytical lighting contribution
	vec3 diffuse_contrib = (1.0 - F) * diffuse(pbr_info);
	vec3 specular_contrib = F * G * D / (4.0 * NdotL * NdotV);

	// obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	vec3 radiance = lighting_ubo.directional_light.color;
	vec3 color = NdotL * radiance * (diffuse_contrib + specular_contrib);

	// calculate lighting contribution from image based lighting source (IBL)
	color += getIBLContribution(pbr_info, n, r);

	// occlusion/emissive color
	color = color * occlusion + emissive_color;

	// get alpha from base_color's alpha
	o_color = vec4(color, base_color.a);
	o_color = vec4(getIBLContribution(pbr_info, n, r), base_color.a);
}