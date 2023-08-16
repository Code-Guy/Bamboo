#pragma once

#include "light_component.h"
#include "runtime/resource/asset/texture_cube.h"

namespace Bamboo
{
	class SkyLightComponent : public LightComponent, public IAssetRef
	{
	public:
		void setTextureCube(std::shared_ptr<TextureCube>& texture_cube);
		std::shared_ptr<TextureCube> getTextureCube() { return m_texture_cube; }

	private:
		REGISTER_REFLECTION(LightComponent)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("light", cereal::base_class<LightComponent>(this)));
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
		}

		virtual void bindRefs() override;

		std::shared_ptr<TextureCube> m_texture_cube;
	};
}