#include "material.h"
#include "runtime/function/global/runtime_context.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::Material>("Material")
	 .constructor<>()
	 .property("m_base_color_texure", &Bamboo::Material::m_base_color_texure)
	 .property("m_metallic_roughness_texure", &Bamboo::Material::m_metallic_roughness_texure)
	 .property("m_normal_texure", &Bamboo::Material::m_normal_texure)
	 .property("m_occlusion_texure", &Bamboo::Material::m_occlusion_texure)
	 .property("m_emissive_texure", &Bamboo::Material::m_emissive_texure);
}

namespace Bamboo
{
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