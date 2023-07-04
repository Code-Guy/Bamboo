#pragma once

#include "runtime/resource/asset/base/texture.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class Texture2D : public Texture, public Asset
	{
	public:
		virtual void inflate() override;

		std::vector<uint8_t> m_image_data;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("texture", cereal::base_class<Texture>(this)));
			ar(cereal::make_nvp("image_data", m_image_data));
		}
	};
}

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)