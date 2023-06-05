#pragma once

#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class SkeletalMesh : public Mesh, public Asset
	{
	public:
		SkeletalMesh(const URL& url);

		std::vector<SkeletalVertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};
}