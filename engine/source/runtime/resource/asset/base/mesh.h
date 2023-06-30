#pragma once

#include "runtime/resource/asset/base/sub_mesh.h"

struct StaticVertex
{
	glm::vec3 m_position;
	glm::vec2 m_tex_coord;
	glm::vec3 m_normal;

private:
	friend class cereal::access;
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(m_position, m_tex_coord, m_normal);
	}
};

struct SkeletalVertex : public StaticVertex
{
	glm::ivec4 m_bones;
	glm::vec4 m_weights;

private:
	friend class cereal::access;
	template<class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::base_class<StaticVertex>(this));
		ar(m_bones, m_weights);
	}
};

struct StaticMeshUBO
{
	glm::mat4 padding;
};

#define MAX_BONE_NUM 128
struct SkeletalMeshUBO
{
	glm::mat4 gBones[MAX_BONE_NUM];
};

namespace Bamboo
{
	class Mesh
	{
	public:
		virtual ~Mesh();

		std::vector<SubMesh> m_sub_meshes;
		std::vector<uint32_t> m_indices;

		VmaBuffer m_vertex_buffer;
		VmaBuffer m_index_buffer;
		std::vector<VmaBuffer> m_uniform_buffers;
		
	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_sub_meshes);
			ar(m_indices);
		}
	};
}