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
		void loadFromGltf(const tinygltf::Image& gltf_image, const tinygltf::Sampler& gltf_sampler);

	protected:
		virtual void inflate() override;

	private:
		VkFilter getVkFilterFromGltf(int gltf_filter);
		VkSamplerAddressMode getVkAddressModeFromGltf(int gltf_wrap);

		friend class cereal::access;
		template<class Archive>
		void save(Archive& archive) const
		{
			archive(m_image_data);
		}

		template<class Archive>
		void load(Archive& archive)
		{
			archive(m_image_data);

			inflate();
		}

		std::vector<uint8_t> m_image_data;
	};

	template <class Archive>
	struct cereal::specialize<Archive, Texture2D, cereal::specialization::member_load_save> {};
}