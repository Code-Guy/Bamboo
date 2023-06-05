#pragma once

#include "runtime/resource/asset/base/texture.h"
#include "runtime/resource/asset/base/asset.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>

namespace Bamboo
{
	class Texture2D : public Texture, public Asset
	{
	public:
		Texture2D(const URL& url);

		void loadFromGltf(const tinygltf::Image& gltf_image, const tinygltf::Sampler& gltf_sampler);
		void setTextureType(TextureType texture_type);

	protected:
		virtual void inflate() override;

	private:
		VkFilter getVkFilterFromGltf(int gltf_filter);
		VkSamplerAddressMode getVkAddressModeFromGltf(int gltf_wrap);

		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(cereal::base_class<Texture>(this));
			ar(m_image_data.size());
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);

			inflate();
		}

		std::vector<uint8_t> m_image_data;
	};
}