#include "skeletal_mesh_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SkeletalMeshComponent>("SkeletalMeshComponent")
	 .constructor<>()
	 .property("m_skeletal_mesh", &Bamboo::SkeletalMeshComponent::m_skeletal_mesh);
}

namespace Bamboo
{
	void SkeletalMeshComponent::setSkeletalMesh(std::shared_ptr<SkeletalMesh>& skeletal_mesh)
	{
		m_skeletal_mesh = skeletal_mesh;
		m_ref_urls["m_skeletal_mesh"] = m_skeletal_mesh->getURL();
	}

	void SkeletalMeshComponent::bindRefs()
	{
		if (m_skeletal_mesh)
		{
			return;
		}

		const auto& iter = m_ref_urls.begin();
		std::shared_ptr<SkeletalMesh> skeletal_mesh = g_runtime_context.assetManager()->loadAsset<SkeletalMesh>(iter->second);
		rttr::type::get(*this).get_property(iter->first).set_value(*this, skeletal_mesh);
	}

}