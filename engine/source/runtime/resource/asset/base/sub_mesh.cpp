#include "sub_mesh.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SubMesh>("SubMesh")
	 .constructor<>()
	 .property("m_material", &Bamboo::SubMesh::m_material);
}

namespace Bamboo
{
	void SubMesh::bindRefs()
	{
		if (m_material)
		{
			return;
		}

		const auto& iter = m_ref_urls.begin();
		std::shared_ptr<Material> material = g_runtime_context.assetManager()->loadAsset<Material>(iter->second);
		rttr::type::get(*this).get_property(iter->first).set_value(*this, material);
	}

}