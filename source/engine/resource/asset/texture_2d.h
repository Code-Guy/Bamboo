#pragma once

#include "engine/resource/asset/base/texture.h"
#include "engine/resource/asset/base/asset.h"

namespace Bamboo
{
	enum class ETextureCompressionMode
	{
		None, ETC1S, ASTC, ZSTD
	};

	class Texture2D : public Texture, public Asset
	{
	public:
		Texture2D();

		virtual void inflate() override;

		ETextureCompressionMode m_compression_mode;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("texture", cereal::base_class<Texture>(this)));
			ar(cereal::make_nvp("compression_mode", m_compression_mode));
		}

		bool compress();
		bool transcode();
	};
}