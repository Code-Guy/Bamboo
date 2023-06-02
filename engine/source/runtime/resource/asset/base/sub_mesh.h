#pragma once

#include "runtime/resource/asset/material.h"
#include "runtime/core/math/bounding_box.h"

namespace Bamboo
{
	class SubMesh
	{
	public:
		uint32_t m_first_index;
		uint32_t m_index_count;
		uint32_t m_vertex_count;
		
		std::shared_ptr<Material> m_material;
		BoundingBox m_bounding_box;
	};
}