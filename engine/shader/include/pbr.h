#ifndef PBR
#define PBR

#include "hdr.h"
#include "host_device.h"

// ibl textures
layout(set = 0, binding = 5) uniform samplerCube irradiance_texture_sampler;
layout(set = 0, binding = 6) uniform samplerCube prefilter_texture_sampler;
layout(set = 0, binding = 7) uniform sampler2D brdf_lut_texture_sampler;

// shadow textures
layout(set = 0, binding = 8) uniform sampler2DArray directional_light_shadow_texture_sampler;
layout(set = 0, binding = 9) uniform samplerCube point_light_shadow_texture_samplers[MAX_POINT_LIGHT_NUM];
layout(set = 0, binding = 10) uniform sampler2D spot_light_shadow_texture_samplers[MAX_SPOT_LIGHT_NUM];

// lighting ubo
layout(set = 0, binding = 11) uniform _LightingUBO { LightingUBO lighting_ubo; };

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

vec3 getLightContribution(PBRInfo pbr_info, vec3 n, vec3 v, vec3 l, vec3 c)
{
	vec3 h = normalize(l + v);
	pbr_info.NdotL = clamp(dot(n, l), 0.001, 1.0);
	pbr_info.NdotH = clamp(dot(n, h), 0.0, 1.0);
	pbr_info.VdotH = clamp(dot(v, h), 0.0, 1.0);

	// calculate the shading terms for the microfacet specular shading model
	vec3 F = specularReflection(pbr_info);
	float G = geometricOcclusion(pbr_info);
	float D = microfacetDistribution(pbr_info);

	// calculation of analytical lighting contribution
	vec3 diffuse_contrib = (1.0 - F) * diffuse(pbr_info);
	vec3 specular_contrib = F * G * D / (4.0 * pbr_info.NdotL * pbr_info.NdotV);

	// obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
	return pbr_info.NdotL * c * (diffuse_contrib + specular_contrib);
}

const mat4 k_shadow_bias_mat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 shadow_coord, vec2 offset, uint cascade_index)
{
	if (shadow_coord.z > 0.0 && shadow_coord.z < 1.0) 
	{
		float depth = texture(directional_light_shadow_texture_sampler, 
			vec3(shadow_coord.xy + offset, cascade_index)).r;
		if (depth < shadow_coord.z - DIRECTIONAL_LIGHT_SHADOW_BIAS) 
		{
			return 0.0;
		}
	}
	return 1.0;
}

float filterPCF(vec4 shadow_coord, uint cascade_index)
{
	ivec2 dim = textureSize(directional_light_shadow_texture_sampler, 0).xy;
	float dx = PCF_DELTA_SCALE / float(dim.x);
	float dy = PCF_DELTA_SCALE / float(dim.y);

	int count = 0;
	float shadow = 0.0;
	for (int x = -PCF_SAMPLE_RANGE; x <= PCF_SAMPLE_RANGE; ++x) 
	{
		for (int y = -PCF_SAMPLE_RANGE; y <= PCF_SAMPLE_RANGE; ++y) 
		{
			shadow += textureProj(shadow_coord, vec2(dx * x, dy * y), cascade_index);
			count++;
		}
	}
	return shadow / count;
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
	vec3 r = reflect(-v, n);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);

	// construct pbr info structure
	PBRInfo pbr_info;
	pbr_info.NdotV = NdotV;
	pbr_info.perceptual_roughness = perceptual_roughness;
	pbr_info.alpha_roughness = alpha_roughness;
	pbr_info.metallic = mat_info.metallic;
	pbr_info.reflectance0 = specularEnvironmentR0;
	pbr_info.reflectance90 = specularEnvironmentR90;
	pbr_info.diffuse_color = diffuse_color;
	pbr_info.specular_color = specular_color;

	// calculate light contribution
	vec3 color = vec3(0.0);

	// directional light
	if (bool(lighting_ubo.has_directional_light))
	{
		DirectionalLight directional_light = lighting_ubo.directional_light;

		float shadow = 1.0;
		if (bool(directional_light.cast_shadow))
		{
			uint cascade_index = 0;
			vec3 view_pos = (lighting_ubo.camera_view * vec4(mat_info.position, 1.0)).xyz;
			for(uint i = 0; i < SHADOW_CASCADE_NUM - 1; ++i)
			{
				if(view_pos.z < directional_light.cascade_splits[i]) 
				{	
					cascade_index = i + 1;
				}
			}
			vec4 shadow_coord = (k_shadow_bias_mat * directional_light.cascade_view_projs[cascade_index]) * vec4(mat_info.position, 1.0);
			shadow_coord = shadow_coord / shadow_coord.w;
			shadow = filterPCF(shadow_coord, cascade_index);
		}
		
		color += getLightContribution(pbr_info, n, v, -directional_light.direction, directional_light.color) * shadow;
	}

	// point lights
	for (int i = 0; i < lighting_ubo.point_light_num; ++i)
	{
		PointLight point_light = lighting_ubo.point_lights[i];
		
		float distance = distance(point_light.position, mat_info.position);
		if (distance < point_light.radius)
		{
			float attenuation = 1.0 / (1.0 + point_light.linear_attenuation * distance + 
	    		    point_light.quadratic_attenuation * (distance * distance));
			vec3 c = point_light.color * attenuation;
			vec3 l = normalize(point_light.position - mat_info.position);

			float shadow = 1.0;
			if (bool(point_light.cast_shadow))
			{
				vec3 sample_vector = mat_info.position - point_light.position;
				float depth = texture(point_light_shadow_texture_samplers[i], sample_vector).x;
				if (length(sample_vector) > depth)
				{
					shadow = 0.0;
				}
			}

			color += getLightContribution(pbr_info, n, v, l, c) * shadow;
		}
	}

	// spot lights
	for (int i = 0; i < lighting_ubo.spot_light_num; ++i)
	{
		SpotLight spot_light = lighting_ubo.spot_lights[i];
		PointLight point_light = spot_light._pl;

		float distance = distance(point_light.position, mat_info.position);
		if (distance < point_light.radius)
		{
			float pl_attenuation = 1.0 / (1.0 + point_light.linear_attenuation * distance + 
	    		    point_light.quadratic_attenuation * (distance * distance));
			float theta = dot(normalize(mat_info.position - point_light.position), spot_light.direction);
			float epsilon = point_light.padding0 - point_light.padding1;
			float sl_attenuation = clamp((theta - point_light.padding1) / epsilon, 0.0, 1.0); 

			vec3 c = point_light.color * pl_attenuation * sl_attenuation;
			vec3 l = normalize(point_light.position - mat_info.position);

			float shadow = 1.0;
			if (bool(point_light.cast_shadow))
			{
				vec4 shadow_coord = (k_shadow_bias_mat * spot_light.view_proj) * vec4(mat_info.position, 1.0);
				shadow_coord = shadow_coord / shadow_coord.w;

				if (shadow_coord.z > 0.0 && shadow_coord.z < 1.0) 
				{
					float depth = texture(spot_light_shadow_texture_samplers[i], shadow_coord.xy).r;
					if (depth < shadow_coord.z - SPOT_LIGHT_SHADOW_BIAS) 
					{
						shadow = 0.0;
					}
				}
			}

			color += getLightContribution(pbr_info, n, v, l, c) * shadow;
		}
	}

	// calculate ibl contribution
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