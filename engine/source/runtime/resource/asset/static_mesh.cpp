#include "static_mesh.h"

namespace Bamboo
{

	void StaticMesh::inflate()
	{
		createVertexBuffer(m_vertices.size() * sizeof(m_vertices[0]), m_vertices.data(), m_vertex_buffer);
		createIndexBuffer(m_indices, m_index_buffer);
	}

}