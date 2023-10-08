#include "material.h"
#include "engine/function/global/engine_context.h"
#include "engine/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::Material>("Material")
	 .property("base_color_texure", &Bamboo::Material::m_base_color_texure)
	 .property("metallic_roughness_occlusion_texure", &Bamboo::Material::m_metallic_roughness_occlusion_texure)
	 .property("normal_texure", &Bamboo::Material::m_normal_texure)
	 .property("emissive_texure", &Bamboo::Material::m_emissive_texure);
}

CEREAL_REGISTER_TYPE(Bamboo::Material)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Material)

namespace Bamboo
{

	void Material::bindRefs()
	{
		BIND_ASSET(m_base_color_texure, Texture2D)
		BIND_ASSET(m_metallic_roughness_occlusion_texure, Texture2D)
		BIND_ASSET(m_normal_texure, Texture2D)
		BIND_ASSET(m_emissive_texure, Texture2D)
	}

}