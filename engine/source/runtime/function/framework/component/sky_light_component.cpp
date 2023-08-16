#include "sky_light_component.h"
#include "runtime/resource/asset/asset_manager.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::SkyLightComponent>("SkyLightComponent")
	 .property("m_texture_cube", &Bamboo::SkyLightComponent::m_texture_cube);
}

CEREAL_REGISTER_TYPE(Bamboo::SkyLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::LightComponent, Bamboo::SkyLightComponent)

namespace Bamboo
{

	void SkyLightComponent::setTextureCube(std::shared_ptr<TextureCube>& texture_cube)
	{
		REF_ASSET(m_texture_cube, texture_cube)
	}

	void SkyLightComponent::bindRefs()
	{
		BIND_ASSET(m_texture_cube, TextureCube)
	}

}