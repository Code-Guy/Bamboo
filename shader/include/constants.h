#ifndef CONSTANTS
#define CONSTANTS

#define PI 3.1415926
#define TWO_PI (PI * 2.0)
#define HALF_PI (PI * 0.5)

#define INVALID_BONE -1
#define MAX_BONE_NUM 128
#define BONE_NUM_PER_VERTEX 4

#define STD_GAMMA 2.2
#define DIELECTRIC_F0 0.04
#define TONEMAP_EXPOSURE 4.5
#define EPSILON 0.001

#define MAX_POINT_LIGHT_NUM 8
#define MAX_SPOT_LIGHT_NUM 8
#define SHADOW_CASCADE_NUM 4
#define SHADOW_FACE_NUM 6
#define MIN_SHADOW_ALPHA 0.001
#define MIN_OUTLINE_ALPHA 0.001
#define DIRECTIONAL_LIGHT_SHADOW_BIAS 0.002
#define POINT_LIGHT_SHADOW_BIAS 0.001
#define SPOT_LIGHT_SHADOW_BIAS 0.001
#define PCF_DELTA_SCALE 0.75
#define PCF_SAMPLE_RANGE 1

#define OUTLINE_THICKNESS 2
#define DEBUG_SHADER_DEPTH_MULTIPLIER 0.02

#ifndef __cplusplus
bool is_nearly_equal(float a, float b)
{
    return abs(a - b) < EPSILON;
}

bool is_nearly_zero(float v)
{
    return abs(v) < EPSILON;
}
#endif

#endif