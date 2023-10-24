#pragma once

#include <basisu/encoder/basisu_enc.h>
#include <basisu/transcoder/basisu_containers.h>

#include "engine/resource/asset/base/texture.h"
#include "engine/resource/asset/base/asset.h"

namespace Bamboo
{
	class Texture2D : public Texture, public Asset
	{
	public:
		virtual void inflate() override;

		// Texture Compression
		bool compressTexture(const basisu::image& source_image, float compress_intensity = 1.0f);
		bool compressTexture(const uint8_t* p_image_RGBA, uint32_t width, uint32_t height, float compress_intensity = 1.0f);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("texture", cereal::base_class<Texture>(this)));
		}

		// Unpack Compressed Texture
		float m_compress_intensity = 1.0f;
		bool unpackKTX(std::vector<uint8_t>& image_data);
		bool unpackUASTC(const std::vector<uint8_t>& image_data, std::vector<uint8_t>& output_data);
	};
}