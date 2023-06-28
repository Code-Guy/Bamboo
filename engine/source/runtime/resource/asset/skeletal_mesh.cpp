#include "skeletal_mesh.h"

namespace Bamboo
{

	void SkeletalMesh::inflate()
	{
		VulkanUtil::createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		VulkanUtil::createIndexBuffer(m_indices, m_index_buffer);
	}

}