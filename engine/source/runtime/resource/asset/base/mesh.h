#pragma once

#include "runtime/resource/asset/base/sub_mesh.h"

struct StaticVertex
{
	glm::vec3 position;
	glm::vec2 tex_coord;
	glm::vec3 normal;
};

struct SkeletalVertex : public StaticVertex
{
	glm::ivec4 bones;
	glm::vec4 weights;
};

namespace Bamboo
{
	class Mesh
	{
	public:
		std::vector<SubMesh> m_sub_meshes;
	};
}