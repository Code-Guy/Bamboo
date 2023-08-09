#pragma once

#include "runtime/resource/asset/base/texture.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class TextureCube : public Texture, public Asset
	{
	public:
		virtual void inflate() override;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("texture", cereal::base_class<Texture>(this)));
		}
	};
}