#include "static_mesh_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::StaticMeshComponent>("StaticMeshComponent")
	 .property("m_static_mesh", &Bamboo::StaticMeshComponent::m_static_mesh);
}

CEREAL_REGISTER_TYPE(Bamboo::StaticMeshComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::StaticMeshComponent)

namespace Bamboo
{
	void StaticMeshComponent::setStaticMesh(std::shared_ptr<StaticMesh>& static_mesh)
	{
		REF_ASSET(m_static_mesh, static_mesh)
	}

	void StaticMeshComponent::bindRefs()
	{
		BIND_ASSET(m_static_mesh, StaticMesh)
	}

}