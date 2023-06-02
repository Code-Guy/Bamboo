#pragma once

#include "runtime/resource/asset/base/texture.h"
#include "runtime/resource/asset/base/asset.h"
#include <tinygltf/tiny_gltf.h>

namespace Bamboo
{
	class Texture2D : public Texture, public Asset
	{
	public:
		void loadFromGltf(const tinygltf::Image& gltf_image, const tinygltf::Sampler& gltf_sampler);

		virtual void inflate() override;

	private:
		VkFilter getVkFilterFromGltf(int gltf_filter);
		VkSamplerAddressMode getVkAddressModeFromGltf(int gltf_wrap);

		std::vector<uint8_t> m_image_data;
	};
}