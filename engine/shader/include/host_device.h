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

struct StaticMeshUBO
{
	mat4 padding;
};

struct SkeletalMeshUBO
{
	mat4 bone_matrices[MAX_BONE_NUM];
};

struct VertPCO
{
    mat4 m;
    mat4 mvp;
};

struct FragPCO
{
    vec3 camera_pos; float padding0;
    vec3 light_dir; float padding1;
    vec3 base_color_factor; int has_base_color_texture;
    vec3 emissive_factor; int has_emissive_texture;
    float m_metallic_factor;
    float m_roughness_factor;
};

#endif