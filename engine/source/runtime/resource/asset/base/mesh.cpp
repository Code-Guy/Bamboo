#include "mesh.h"

namespace Bamboo
{

	Mesh::~Mesh()
	{
		m_vertex_buffer.destroy();
		m_index_buffer.destroy();
	}

}