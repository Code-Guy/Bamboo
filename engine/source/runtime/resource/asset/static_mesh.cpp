#include "static_mesh.h"

namespace Bamboo
{

	StaticMesh::StaticMesh(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::StaticMesh;
		m_archive_type = EArchiveType::Json;
	}

	void StaticMesh::inflate()
	{
		createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		createIndexBuffer(m_indices, m_index_buffer);
	}

}