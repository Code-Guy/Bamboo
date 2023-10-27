#include "texture_2d.h"
#include "engine/core/base/macro.h"
#include "engine/platform/timer/timer.h"
#include <ktx.h>

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)

namespace Bamboo
{

	Texture2D::Texture2D()
	{
		m_compression_mode = ETextureCompressionMode::ETC1S;
	}

	void Texture2D::inflate()
	{
		m_layers = 1;
		m_mip_levels = m_texture_type == ETextureType::BaseColor ? VulkanUtil::calcMipLevel(m_width, m_height) : 1;

		bool b_need_compress = m_image_data.size() == m_width * m_height * VulkanUtil::calcFormatSize(getFormat());
		if (b_need_compress)
		{
			compress();
		}
		transcode();
		upload();
	}

	bool Texture2D::compress()
	{
		StopWatch stop_watch;
		stop_watch.start();

		ktxTexture* ktx_texture;
		ktx_error_code_e result;
		ktxTextureCreateInfo ktx_texture_ci = {};
		ktx_texture_ci.vkFormat = getFormat();
		ktx_texture_ci.baseWidth = m_width;
		ktx_texture_ci.baseHeight = m_height;
		ktx_texture_ci.baseDepth = 1;
		ktx_texture_ci.numDimensions = 2;
		ktx_texture_ci.numLevels = 1;
		ktx_texture_ci.numLayers = m_layers;
		ktx_texture_ci.numFaces = 1;
		ktx_texture_ci.isArray = KTX_FALSE;
		ktx_texture_ci.generateMipmaps = KTX_TRUE;
		result = ktxTexture2_Create(&ktx_texture_ci, ktxTextureCreateStorageEnum::KTX_TEXTURE_CREATE_ALLOC_STORAGE, (ktxTexture2**)&ktx_texture);
		result = ktxTexture_SetImageFromMemory(ktx_texture, 0, 0, 0, m_image_data.data(), m_image_data.size());
		ASSERT(result == KTX_SUCCESS, "failed to create ktx_texture: {}", ktxErrorString(result));

		if (m_compression_mode != ETextureCompressionMode::ZSTD)
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
			basis_params.uastcFlags = KTX_PACK_UASTC_LEVEL_FASTEST;
			basis_params.uastcRDO = true;
			basis_params.uastcRDOQualityScalar = 32;

			result = ktxTexture2_CompressBasisEx((ktxTexture2*)ktx_texture, &basis_params);
			ASSERT(result == KTX_SUCCESS, "failed to compress ktx_texture: {}", ktxErrorString(result));
		}

		if (m_compression_mode == ETextureCompressionMode::ASTC)
		{
			const uint32_t k_zstd_compression_level = 10;
			result = ktxTexture2_DeflateZstd((ktxTexture2*)ktx_texture, k_zstd_compression_level);
			ASSERT(result == KTX_SUCCESS, "failed to deflate ktx_texture: {}", ktxErrorString(result));
		}
		
		ktx_size_t compressed_size = 0;
		uint32_t original_size = m_image_data.size();

		ktxTexture_WriteToMemory(ktx_texture, (ktx_uint8_t**)&m_image_data.front(), &compressed_size);
		m_image_data.resize(compressed_size);
		m_image_data.shrink_to_fit();

		LOG_INFO("finished to compress ktx_texture texture. elapsed time: {}ms, compression ratio: {}", 
			stop_watch.stopMs(), (float)(compressed_size / original_size));

		return true;
	}

	bool Texture2D::transcode()
	{
		ktxTexture* ktx_texture;
		ktxResult result = ktxTexture_CreateFromMemory(m_image_data.data(), m_image_data.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
		ASSERT(result == KTX_SUCCESS, "failed to inflate texture cube");

		//if (ktxTexture2_NeedsTranscoding())
		{

		}
		return true;
	}

	bool Texture2D::upload()
	{

	}

}
