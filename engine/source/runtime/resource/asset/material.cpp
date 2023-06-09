#include "material.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

namespace Bamboo
{
	RTTR_REGISTRATION
	{
	rttr::registration::class_<Material>("Material")
		 .constructor<>()
		 .property("m_base_color_texure", &Material::m_base_color_texure)
		 .property("m_metallic_roughness_texure", &Material::m_metallic_roughness_texure)
		 .property("m_normal_texure", &Material::m_normal_texure)
		 .property("m_occlusion_texure", &Material::m_occlusion_texure)
		 .property("m_emissive_texure", &Material::m_emissive_texure);
	}

	void Material::bindRefs()
	{
		if (m_base_color_texure)
		{
			return;
		}

		for (const auto& iter : m_ref_urls)
		{
			std::shared_ptr<Texture2D> texture = g_runtime_context.assetManager()->loadAsset<Texture2D>(iter.second);
			rttr::type::get(*this).get_property(iter.first).set_value(*this, texture);
		}
	}

}