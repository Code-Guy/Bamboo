#pragma once

#include "component.h"
#include "runtime/resource/asset/static_mesh.h"

namespace Bamboo
{
	class StaticMeshComponent : public Component
	{
	public:
		StaticMeshComponent(std::shared_ptr<class Entity> parent) : Component(parent) {}
		virtual ~StaticMeshComponent() = default;

		std::shared_ptr<StaticMesh> getStaticMesh() { return m_static_mesh; }

	private:
		std::shared_ptr<StaticMesh> m_static_mesh;
	};
}