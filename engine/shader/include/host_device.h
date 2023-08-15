#ifndef HOST_DEVICE
#define HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>

// glsl types
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

#include "shader_constants.h"

struct BoneUBO
{
	mat4 bone_matrices[MAX_BONE_NUM];
};

struct TransformPCO
{
    mat4 m;
    mat4 nm;
    mat4 mvp;
};

struct MaterialPCO
{
    vec4 base_color_factor;
    vec4 emissive_factor;
	float m_metallic_factor;
	float m_roughness_factor;

    int has_base_color_texture; 
    int has_emissive_texture;
    int has_metallic_roughness_texture;
    int has_normal_texture;
    int has_occlusion_texture;
};

struct LightingUBO
{
    vec3 camera_pos; float padding0;
    vec3 light_dir; float padding1;
    mat4 inv_view_proj;
};

#endif