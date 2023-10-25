#include "mesh_collider_component.h"
#include "engine/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::MeshColliderComponent>("MeshColliderComponent")
	.property("static_mesh", &Bamboo::MeshColliderComponent::m_static_mesh);
}

CEREAL_REGISTER_TYPE(Bamboo::MeshColliderComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::ColliderComponent, Bamboo::MeshColliderComponent)

namespace Bamboo
{

	MeshColliderComponent::MeshColliderComponent()
	{
		m_type = EColliderType::Mesh;
	}

	void MeshColliderComponent::setStaticMesh(std::shared_ptr<StaticMesh>& static_mesh)
	{
		REF_ASSET(m_static_mesh, static_mesh)
	}

	void MeshColliderComponent::bindRefs()
	{
		BIND_ASSET(m_static_mesh, StaticMesh)
	}
}