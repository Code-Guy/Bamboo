#pragma once

#include "runtime/resource/asset/base/sub_mesh.h"

struct StaticVertex
{
	glm::vec3 m_position;
	glm::vec2 m_tex_coord;
	glm::vec3 m_normal;
};

struct SkeletalVertex : public StaticVertex
{
	glm::ivec4 m_bones;
	glm::vec4 m_weights;
};

namespace Bamboo
{
	class Mesh
	{
	public:
		virtual ~Mesh();

		std::vector<SubMesh> m_sub_meshes;
		std::vector<uint32_t> m_indices;

	protected:
		VmaBuffer m_vertex_buffer;
		VmaBuffer m_index_buffer;

	private:
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(m_sub_meshes);
			ar(m_indices.size());
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);
		}
	};
}