#include "static_mesh_component.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::StaticMeshComponent>("StaticMeshComponent")
	 .constructor<>()
	 .property("m_static_mesh", &Bamboo::StaticMeshComponent::m_static_mesh);
}

CEREAL_REGISTER_TYPE(Bamboo::StaticMeshComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::StaticMeshComponent)

namespace Bamboo
{
	void StaticMeshComponent::setStaticMesh(std::shared_ptr<StaticMesh>& static_mesh)
	{
		m_static_mesh = static_mesh;
		m_ref_urls["m_static_mesh"] = m_static_mesh->getURL();
	}

	void StaticMeshComponent::bindRefs()
	{
		if (m_static_mesh)
		{
			return;
		}

		const auto& iter = m_ref_urls.begin();
		std::shared_ptr<StaticMesh> static_mesh = g_runtime_context.assetManager()->loadAsset<StaticMesh>(iter->second);
		rttr::type::get(*this).get_property(iter->first).set_value(*this, static_mesh);
	}

}