#ifndef PBR
#define PBR

#include "hdr.h"
#include "host_device.h"

layout(set = 0, binding = 5) uniform samplerCube irradiance_texture_sampler;
layout(set = 0, binding = 6) uniform samplerCube prefilter_texture_sampler;
layout(set = 0, binding = 7) uniform sampler2D brdf_lut_texture_sampler;

layout(set = 0, binding = 8) uniform _LightingUBO { LightingUBO lighting_ubo; };

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

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
vec3 getIBLContribution(PBRInfo pbr_info, vec3 n, vec3 r)
{
	float lod = pbr_info.perceptual_roughness * lighting_ubo.sky_light.prefilter_mip_levels;

	// retrieve a scale and bias to F0. See [1], Figure 3
	vec3 brdf = (texture(brdf_lut_texture_sampler, vec2(pbr_info.NdotV, 1.0 - pbr_info.perceptual_roughness))).rgb;
	vec3 diffuse_light = srgb_to_linear(tonemap(texture(irradiance_texture_sampler, n).rgb));
	vec3 specular_light = srgb_to_linear(tonemap(textureLod(prefilter_texture_sampler, r, lod).rgb));

	vec3 diffuse = diffuse_light * pbr_info.diffuse_color;
	vec3 specular = specular_light * (pbr_info.specular_color * brdf.x + brdf.y);

	return (diffuse + specular) * lighting_ubo.sky_light.color;
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

vec4 calc_pbr(MaterialInfo mat_info)
{
	float perceptual_roughness = mat_info.roughness;
	float alpha_roughness = perceptual_roughness * perceptual_roughness;

	// calculate diffuse_color and specular_color
	vec3 f0 = vec3(DIELECTRIC_F0);
	vec3 diffuse_color = mat_info.base_color.rgb * (vec3(1.0) - f0);
	diffuse_color *= 1.0 - mat_info.metallic;
	vec3 specular_color = mix(f0, mat_info.base_color.rgb, mat_info.metallic);
	
	// calculate reflectance
	float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);

	// for typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
	// for very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
	vec3 specularEnvironmentR0 = specular_color.rgb;
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR90 = reflectance90 * vec3(1.0);

	// surface/camera/light directions
	vec3 n = mat_info.normal;
	vec3 v = normalize(lighting_ubo.camera_pos - mat_info.position);
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
		mat_info.metallic,
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
	vec3 radiance = bool(lighting_ubo.has_directional_light) ? lighting_ubo.directional_light.color : vec3(0.0);
	vec3 color = NdotL * radiance * (diffuse_contrib + specular_contrib);

	// calculate lighting contribution from image based lighting source (IBL)
	if (bool(lighting_ubo.has_sky_light))
	{
		color += getIBLContribution(pbr_info, n, r);
	}
	
	// occlusion/emissive color
	color = color * mat_info.occlusion + mat_info.emissive_color.xyz;

	// get alpha from base_color's alpha
	return vec4(color, mat_info.base_color.a);
}

#endif