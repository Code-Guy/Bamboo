// Generates an irradiance cube from an environment map using convolution

#version 450

layout (location = 0) in vec3 f_uvw;
layout (location = 0) out vec4 f_color;
layout (binding = 0) uniform samplerCube env_tex;

layout(push_constant) uniform PCO 
{
	layout (offset = 64) float delta_phi;
	layout (offset = 68) float delta_theta;
} pco;

#define PI 3.1415926536
#define TWO_PI (PI * 2.0)
#define HALF_PI (PI * 0.5)

void main()
{
	vec3 n = normalize(f_uvw);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, n));
	up = cross(n, right);

	vec3 color = vec3(0.0);
	uint samples = 0u;
	for (float phi = 0.0; phi < TWO_PI; phi += pco.delta_phi) 
	{
		for (float theta = 0.0; theta < HALF_PI; theta += pco.delta_theta) 
		{
			vec3 tangent_vector = cos(phi) * right + sin(phi) * up;
			vec3 sample_vector = cos(theta) * n + sin(theta) * tangent_vector;
			color += texture(env_tex, sample_vector).rgb * cos(theta) * sin(theta);
			samples++;
		}
	}

	f_color = vec4(PI * color / float(samples), 1.0);
}
