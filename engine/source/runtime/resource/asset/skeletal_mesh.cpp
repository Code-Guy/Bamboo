#include "skeletal_mesh.h"

CEREAL_REGISTER_TYPE(Bamboo::SkeletalMesh)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::SkeletalMesh)

namespace Bamboo
{

	void SkeletalMesh::inflate()
	{
		VulkanUtil::createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		VulkanUtil::createIndexBuffer(m_indices, m_index_buffer);

		m_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
		for (VmaBuffer& uniform_buffer : m_uniform_buffers)
		{
			VulkanUtil::createBuffer(sizeof(SkeletalMeshUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, uniform_buffer);
		}
	}

}