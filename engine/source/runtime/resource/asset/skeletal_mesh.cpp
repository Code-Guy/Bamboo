#include "skeletal_mesh.h"

namespace Bamboo
{

	SkeletalMesh::SkeletalMesh(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::SkeletalMesh;
		m_archive_type = EArchiveType::Json;
	}

	void SkeletalMesh::inflate()
	{
		createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		createIndexBuffer(m_indices, m_index_buffer);
	}

}