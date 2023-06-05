#pragma once

#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class StaticMesh : public Mesh, public Asset
	{
	public:
		StaticMesh(const URL& url);

		std::vector<StaticVertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};
}