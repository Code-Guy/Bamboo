#version 450
#extension GL_GOOGLE_include_directive : enable

#include "host_device.h"

layout(triangles) in;
layout(triangle_strip, max_vertices=SHADOW_FACE_NUM*3) out;

layout(location = 0) in vec3 f_position[];
layout(location = 1) in vec2 f_tex_coord[];
layout(location = 2) in vec3 f_normal[];

layout(binding = 2) uniform _ShadowCubeUBO { ShadowCubeUBO shadow_cube_ubo; };

layout(location = 0) out vec3 g_position;
layout(location = 1) out vec2 g_tex_coord;

void main()
{
    for(int i = 0; i < SHADOW_FACE_NUM; ++i)
    {
        gl_Layer = i; // built-in variable that specifies to which face we render.
        for(int v = 0; v < 3; ++v) // for each triangle vertex
        {
            gl_Position = shadow_cube_ubo.face_view_projs[i] * vec4(f_position[v], 1.0);
            g_position = f_position[v];
            g_tex_coord = f_tex_coord[v];
            EmitVertex();
        }
        EndPrimitive();
    }
}