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

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("texture", cereal::base_class<Texture>(this)));
		}

		bool unpackKTX(std::vector<uint8_t>& image_data);
		bool unpackUASTC(const std::vector<uint8_t>& image_data, std::vector<uint8_t>& output_data);

		bool unpackTexture(basisu::image img);
	};
}