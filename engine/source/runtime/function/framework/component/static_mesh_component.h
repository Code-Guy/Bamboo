#pragma once

#include "component.h"
#include "runtime/resource/asset/static_mesh.h"

namespace Bamboo
{
	class StaticMeshComponent : public Component
	{
	public:
		std::shared_ptr<StaticMesh> getStaticMesh() { return m_static_mesh; }

	private:
		std::shared_ptr<StaticMesh> m_static_mesh;
	};
}