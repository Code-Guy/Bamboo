#include "skeletal_mesh_component.h"
#include "engine/function/global/engine_context.h"
#include "engine/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SkeletalMeshComponent>("SkeletalMeshComponent")
	 .property("skeletal_mesh", &Bamboo::SkeletalMeshComponent::m_skeletal_mesh);
}

CEREAL_REGISTER_TYPE(Bamboo::SkeletalMeshComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::SkeletalMeshComponent)

namespace Bamboo
{
	void SkeletalMeshComponent::setSkeletalMesh(std::shared_ptr<SkeletalMesh>& skeletal_mesh)
	{
		REF_ASSET(m_skeletal_mesh, skeletal_mesh)
	}

	void SkeletalMeshComponent::bindRefs()
	{
		BIND_ASSET(m_skeletal_mesh, SkeletalMesh)
	}

}