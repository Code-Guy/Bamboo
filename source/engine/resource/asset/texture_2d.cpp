#include "texture_2d.h"
#include "engine/core/base/macro.h"
#include <basisu/encoder/basisu_comp.h>

#define MINIZ_HEADER_FILE_ONLY
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <basisu/encoder/basisu_miniz.h>

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)

CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)

namespace Bamboo
{
	void Texture2D::inflate()
	{
		m_layers = 1;
		m_mip_levels = m_texture_type == ETextureType::BaseColor ? VulkanUtil::calcMipLevel(m_width, m_height) : 1;

		// unpack compressed texture
		if (m_compress_intensity != 0.0f)
		{
			std::vector<uint8_t> unpacked_image;
			if (!unpackUASTC(m_image_data, unpacked_image))
			{
				LOG_ERROR("Unpack compressed texture failed");
				return;
			}
			VulkanUtil::createImageViewSampler(m_width, m_height, unpacked_image.data(), m_mip_levels, m_layers, getFormat(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
		}
		else
		{
			VulkanUtil::createImageViewSampler(m_width, m_height, m_image_data.data(), m_mip_levels, m_layers, getFormat(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
		}
	}

	bool Texture2D::compressTexture(const uint8_t* p_image_RGBA, uint32_t width, uint32_t height, float compress_intensity)
	{
		m_compress_intensity = compress_intensity;
		// no compression
		if (compress_intensity == 0.0f)
		{
			m_image_data.resize(width * height * 4);
			memcpy(m_image_data.data(), p_image_RGBA, width * height * 4);
			return true;
		}
		
		basisu::basisu_encoder_init();

		basisu::image_stats stats;
		float uastc_rdo_quality = compress_intensity;
		uint32_t pitch_in_pixels = width;
		size_t data_size = 0;

		// UASTC
		uint32_t flags_and_quality = basisu::cFlagThreaded | basisu::cFlagUASTCRDO| basisu::cFlagUASTC | basisu::cFlagPrintStats | basisu::cFlagPrintStatus;

		void* p_image_data = basis_compress(p_image_RGBA, width, height, pitch_in_pixels, flags_and_quality, uastc_rdo_quality, &data_size, &stats);

		if (!p_image_data)
		{
			LOG_ERROR("basis_compress() failed!");
			return false;
		}

		LOG_DEBUG("UASTC Size: {}, PSNR: {}", static_cast<uint32_t>(data_size), stats.m_basis_rgba_avg_psnr);

		m_image_data.resize(data_size);
		memcpy(m_image_data.data(), p_image_data, data_size);
		basisu::basis_free_data(p_image_data);

		return true;
	}

	bool Texture2D::compressTexture(const basisu::image& source_image, float compress_intensity)
	{
		m_compress_intensity = compress_intensity;
		// no compression
		if (compress_intensity == 0.0f)
		{
			m_image_data.resize(source_image.get_total_pixels() * 4);
			memcpy(m_image_data.data(), source_image.get_ptr(),source_image.get_total_pixels() * 4);
			return true;
		}

		basisu::basisu_encoder_init();

		basisu::vector<basisu::image> source_images(1);
		basisu::image_stats stats;
		float uastc_rdo_quality = compress_intensity;
		size_t data_size = 0;

		// UASTC
		uint32_t flags_and_quality = basisu::cFlagThreaded | basisu::cFlagUASTCRDO| basisu::cFlagUASTC | basisu::cFlagPrintStats | basisu::cFlagPrintStatus;

		source_images[0] = source_image;
		void* p_image_data = basis_compress(source_images, flags_and_quality, uastc_rdo_quality, &data_size, &stats);
		if (!p_image_data)
		{
			LOG_ERROR("basis_compress() failed!");
			return false;
		}
		// printf("UASTC data %p", static_cast<int*>(p_image_data));

		LOG_DEBUG("UASTC Size: {}, PSNR: {}", static_cast<uint32_t>(data_size), stats.m_basis_rgba_avg_psnr);

		m_image_data.resize(data_size);
		memcpy(m_image_data.data(), p_image_data, data_size);
		basisu::basis_free_data(p_image_data);

		return true;
	}

	bool Texture2D::unpackUASTC(const std::vector<uint8_t>& image_data, std::vector<uint8_t>& output_data)
	{
		// init basisu
		basisu::basisu_encoder_init();

		// trans std to basisu
		basisu::vector<uint8_t> basis_file_data;
		for (auto image : image_data)
		{
			basis_file_data.push_back(image);
		}

		// transcoder begins
		basist::basisu_transcoder dec;
		basist::basisu_file_info file_info;
		if (!dec.get_file_info(basis_file_data.data(), (uint32_t)basis_file_data.size(), file_info))
		{
			LOG_ERROR("Failed retrieving Basis file information!");
			return false;
		}

		LOG_DEBUG("  Texture format: {}", (file_info.m_tex_format == basist::basis_tex_format::cUASTC4x4) ? "UASTC" : "ETC1S");

		uint32_t total_texels = 0;
		LOG_DEBUG("Image info:");
		for (uint32_t i = 0; i < file_info.m_total_images; i++)
		{
			basist::basisu_image_info ii;
			if (!dec.get_image_info(basis_file_data.data(), (uint32_t)basis_file_data.size(), ii, i))
			{
				LOG_ERROR("get_image_info() failed!");
				return false;
			}

			LOG_DEBUG("Image {}: MipLevels: {} OrigDim: {}x{}, BlockDim: {}x{}, FirstSlice: {}, HasAlpha: {}", i, ii.m_total_levels, ii.m_orig_width, ii.m_orig_height,
				ii.m_num_blocks_x, ii.m_num_blocks_y, ii.m_first_slice_index, static_cast<uint32_t>(ii.m_alpha_flag));

			total_texels += ii.m_width * ii.m_height;
		}

		LOG_DEBUG("Slice info:");
		for (uint32_t i = 0; i < file_info.m_slice_info.size(); i++)
		{
			const basist::basisu_slice_info& slice_info = file_info.m_slice_info[i];
			LOG_DEBUG("{}: OrigWidthHeight: {}x{}, BlockDim: {}x{}, TotalBlocks: {}, Compressed size: {}, Image: {}, Level: {}, UnpackedCRC16: 0x{}, alpha: {}, iframe: {}",
				i,
				slice_info.m_orig_width, slice_info.m_orig_height,
				slice_info.m_num_blocks_x, slice_info.m_num_blocks_y,
				slice_info.m_total_blocks,
				slice_info.m_compressed_size,
				slice_info.m_image_index, slice_info.m_level_index,
				slice_info.m_unpacked_slice_crc16,
				static_cast<uint32_t>(slice_info.m_alpha_flag),
				static_cast<uint32_t>(slice_info.m_iframe_flag));
		}

		size_t comp_size = 0;
		void* pComp_data = buminiz::tdefl_compress_mem_to_heap(basis_file_data.data(), basis_file_data.size(), &comp_size, buminiz::TDEFL_MAX_PROBES_MASK);// TDEFL_DEFAULT_MAX_PROBES);
		buminiz::mz_free(pComp_data);

		const float basis_bits_per_texel = basis_file_data.size() * 8.0f / total_texels;
		const float comp_bits_per_texel = comp_size * 8.0f / total_texels;

		LOG_DEBUG("Original size: {}, bits per texel: {}Compressed size (Deflate): {}, bits per texel: {}", (uint32_t)basis_file_data.size(), basis_bits_per_texel, (uint32_t)comp_size, comp_bits_per_texel);

		basisu::interval_timer tm;
		tm.start();

		if (!dec.start_transcoding(basis_file_data.data(), (uint32_t)basis_file_data.size()))
		{
			LOG_ERROR("start_transcoding() failed!");
			return false;
		}

		// timer
		const double start_transcoding_time_ms = tm.get_elapsed_ms();
		LOG_DEBUG("start_transcoding time: {} ms", start_transcoding_time_ms);

		basisu::image out_image;

		// Upack UASTC files separarely, to validate we can transcode slices to UASTC and unpack them to pixels.
		// This is a special path because UASTC is not yet a valid transcoder_texture_format, but a lower-level block_format.
		if (file_info.m_tex_format == basist::basis_tex_format::cUASTC4x4)
		{
			for (uint32_t image_index = 0; image_index < file_info.m_total_images; image_index++)
			{
				for (uint32_t level_index = 0; level_index < file_info.m_image_mipmap_levels[image_index]; level_index++)
				{
					basist::basisu_image_level_info level_info;

					if (!dec.get_image_level_info(basis_file_data.data(), (uint32_t)basis_file_data.size(), level_info, image_index, level_index))
					{
						LOG_ERROR("Failed retrieving image level information ({} {})!", image_index, level_index);
						return false;
					}

					basisu::gpu_image gpu_texture_image;
					gpu_texture_image.init(basisu::texture_format::cUASTC4x4, level_info.m_orig_width, level_info.m_orig_height);

					// Fill the buffer with psuedo-random bytes, to help more visibly detect cases where the transcoder fails to write to part of the output.
					basisu::fill_buffer_with_random_bytes(gpu_texture_image.get_ptr(), gpu_texture_image.get_size_in_bytes());

					tm.start();

					if (!dec.transcode_slice(
						basis_file_data.data(), (uint32_t)basis_file_data.size(), 
						level_info.m_first_slice_index, gpu_texture_image.get_ptr(), gpu_texture_image.get_total_blocks(), basist::block_format::cUASTC_4x4, gpu_texture_image.get_bytes_per_block()))
					{
						LOG_ERROR("Failed transcoding image level ({} {}) to UASTC!", image_index, level_index);
						return false;
					}

					double total_transcode_time = tm.get_elapsed_ms();

					LOG_DEBUG("Transcode of image {} level {} res {}x{} format UASTC_4x4 succeeded in {} ms", image_index, level_index, level_info.m_orig_width, level_info.m_orig_height, total_transcode_time);

					// unpack gpu_image to image
					basisu::image img;
					if (!gpu_texture_image.unpack(img))
					{
						LOG_ERROR("Warning: Failed unpacking GPU texture data ({} {}) to UASTC. ", image_index, level_index);
						return false;
					}

					// copy image to output_data
					uint32_t image_size = img.get_total_pixels() * 4;
					output_data.resize(image_size);
					memcpy(output_data.data(), img.get_ptr(), image_size);
					img.clear();
					return true;
				}
			}
		}
		return false;
	}
}
