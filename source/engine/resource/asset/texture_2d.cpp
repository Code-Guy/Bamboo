#include "texture_2d.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"

#include <ktx.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <tinygltf/stb_image_resize2.h>

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)

namespace Bamboo
{

	Texture2D::Texture2D()
	{
		m_compression_mode = ETextureCompressionMode::ASTC;
	}

	void Texture2D::inflate()
	{
		m_layers = 1;
		m_mip_levels = isMipmap() ? VulkanUtil::calcMipLevel(m_width, m_height) : 1;

		if (m_compression_mode == ETextureCompressionMode::None)
		{
			VulkanUtil::createImageViewSampler(m_width, m_height, m_image_data.data(), m_mip_levels, m_layers, 
				getFormat(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
		}
		else
		{
			bool need_compress = m_image_data.size() == m_width * m_height * VulkanUtil::calcFormatSize(getFormat());
			if (need_compress)
			{
				compress();
			}
			transcode();
		}
	}

	bool Texture2D::compress()
	{
		ktxTexture* ktx_texture;
		ktx_error_code_e result;
		ktxTextureCreateInfo ktx_texture_ci = {};
		ktx_texture_ci.vkFormat = getFormat();
		ktx_texture_ci.baseWidth = m_width;
		ktx_texture_ci.baseHeight = m_height;
		ktx_texture_ci.baseDepth = 1;
		ktx_texture_ci.numDimensions = 2;
		ktx_texture_ci.numLevels = m_mip_levels;
		ktx_texture_ci.numLayers = m_layers;
		ktx_texture_ci.numFaces = 1;
		ktx_texture_ci.isArray = KTX_FALSE;
		ktx_texture_ci.generateMipmaps = KTX_FALSE;
		result = ktxTexture2_Create(&ktx_texture_ci, ktxTextureCreateStorageEnum::KTX_TEXTURE_CREATE_ALLOC_STORAGE, (ktxTexture2**)&ktx_texture);

		// set image data for every mipmap level
		std::vector<std::vector<uint8_t>> mip_image_datas(m_mip_levels);
		uint32_t mip_width = m_width;
		uint32_t mip_height = m_height;
		uint32_t pixel_size = VulkanUtil::calcFormatSize(getFormat());
		for (uint32_t i = 0; i < m_mip_levels; ++i)
		{
			const std::vector<uint8_t>* p_mip_data = nullptr;

			if (i == 0)
			{
				p_mip_data = &m_image_data;
			}
			else
			{
				uint32_t last_mip_width = mip_width;
				uint32_t last_mip_height = mip_height;
				if (mip_width > 1) mip_width >>= 1;
				if (mip_height > 1) mip_height >>= 1;

				mip_image_datas[i].resize(mip_width * mip_height * pixel_size);
				const uint8_t* last_mip_data = i == 1 ? m_image_data.data() : mip_image_datas[i - 1].data();
				if (isSRGB())
				{
					stbir_resize_uint8_srgb(last_mip_data, last_mip_width, last_mip_height, last_mip_width * pixel_size,
						mip_image_datas[i].data(), mip_width, mip_height, mip_width * pixel_size, STBIR_RGBA);
				}
				else
				{
					stbir_resize_uint8_linear(last_mip_data, last_mip_width, last_mip_height, last_mip_width * pixel_size,
						mip_image_datas[i].data(), mip_width, mip_height, mip_width * pixel_size, STBIR_RGBA);
				}

				p_mip_data = &mip_image_datas[i];
			}

			result = ktxTexture_SetImageFromMemory(ktx_texture, i, 0, 0, p_mip_data->data(), p_mip_data->size());
		}
		ASSERT(result == KTX_SUCCESS, "failed to create ktx_texture: {}", ktxErrorString(result));

		if (m_compression_mode == ETextureCompressionMode::ETC1S || m_compression_mode == ETextureCompressionMode::ASTC)
		{
			ktxBasisParams basis_params = {};
			basis_params.structSize = sizeof(ktxBasisParams);
			basis_params.threadCount = std::max(std::thread::hardware_concurrency(), 1u);
			basis_params.uastc = m_compression_mode == ETextureCompressionMode::ASTC;

			// ETC1S
			basis_params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
			basis_params.qualityLevel = 128;
			basis_params.normalMap = m_texture_type == ETextureType::Normal;

			// ASTC
			basis_params.uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT | KTX_PACK_UASTC_FAVOR_BC7_ERROR;
			basis_params.uastcRDO = true;
			basis_params.uastcRDOQualityScalar = 2.0f;

			result = ktxTexture2_CompressBasisEx((ktxTexture2*)ktx_texture, &basis_params);
			ASSERT(result == KTX_SUCCESS, "failed to compress ktx_texture: {}", ktxErrorString(result));
		}

		if (m_compression_mode == ETextureCompressionMode::ASTC || m_compression_mode == ETextureCompressionMode::ZSTD)
		{
			const uint32_t k_zstd_compression_level = 10;
			result = ktxTexture2_DeflateZstd((ktxTexture2*)ktx_texture, k_zstd_compression_level);
			ASSERT(result == KTX_SUCCESS, "failed to deflate ktx_texture: {}", ktxErrorString(result));
		}
		
		ktx_uint8_t* p_compressed_data = nullptr;
		ktx_size_t compressed_size = 0;
		uint32_t original_size = m_image_data.size();

		ktxTexture_WriteToMemory(ktx_texture, &p_compressed_data, &compressed_size);
		ktxTexture_Destroy(ktx_texture);

		m_image_data.resize(compressed_size);
		m_image_data.shrink_to_fit();
		memcpy(m_image_data.data(), p_compressed_data, compressed_size);
		delete[] p_compressed_data;

		LOG_INFO("finished to compress ktx_texture texture, compression ratio: {0:.2f}%", compressed_size * 100.0f / original_size);

		return true;
	}

	bool Texture2D::transcode()
	{
		ktxTexture* ktx_texture;
		ktxResult result = ktxTexture_CreateFromMemory(m_image_data.data(), m_image_data.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
		ASSERT(result == KTX_SUCCESS, "failed to inflate texture");

		VkFormat format = getFormat();
		if (m_compression_mode == ETextureCompressionMode::ETC1S || m_compression_mode == ETextureCompressionMode::ASTC)
		{
			ktxTexture2* ktx2_texture = (ktxTexture2*)ktx_texture;
			ktx_transcode_fmt_e target_format = KTX_TTF_BC7_RGBA;
			if (ktxTexture2_NeedsTranscoding(ktx2_texture))
			{
				result = ktxTexture2_TranscodeBasis(ktx2_texture, target_format, 0);
				ASSERT(result == KTX_SUCCESS, "failed to transcode texture");
				format = (VkFormat)ktx2_texture->vkFormat;
			}
		}

		uploadKtxTexture(ktx_texture, format);
		return true;
	}

}
