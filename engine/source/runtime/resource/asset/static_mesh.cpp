#include "static_mesh.h"

CEREAL_REGISTER_TYPE(Bamboo::StaticMesh)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::StaticMesh)

namespace Bamboo
{

	void StaticMesh::inflate()
	{
		VulkanUtil::createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		VulkanUtil::createIndexBuffer(m_indices, m_index_buffer);
	}

}