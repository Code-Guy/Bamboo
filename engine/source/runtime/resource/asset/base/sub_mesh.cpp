#include "sub_mesh.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

namespace Bamboo
{
	RTTR_REGISTRATION
	{
	rttr::registration::class_<SubMesh>("SubMesh")
		 .constructor<>()
		 .property("m_material", &SubMesh::m_material);
	}

	void SubMesh::onBindRefs()
	{
		for (const auto& iter : m_ref_urls)
		{
			std::shared_ptr<Texture2D> texture = g_runtime_context.assetManager()->loadAsset<Texture2D>(iter.second);
			rttr::type::get(*this).get_property(iter.first).set_value(*this, texture);
		}
	}

}