#include "texture_2d.h"
#include "basisu/basisu_tool.cpp"
#include "engine/core/base/macro.h"

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

		// debug
		// unpackKTX(m_image_data);
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

	bool Texture2D::unpackKTX(std::vector<uint8_t>& image_data)
	{
		basisu_encoder_init();

		basis_data* pGlobal_codebook_data = nullptr;
		basisu::vector<uint8_t> file_data;
		for (auto image : image_data)
		{
			file_data.push_back(image);
		}
		image_data.clear();

		command_line_params opts;
		uint32_t total_unpack_warnings = 0;
		uint32_t total_pvrtc_nonpow2_warnings = 0;
		const bool status = unpack_and_validate_basis_file(
			0,
			"",
			file_data,
			opts,
			nullptr,
			pGlobal_codebook_data,
			total_unpack_warnings,
			total_pvrtc_nonpow2_warnings);

		if (!status)
		{
			delete pGlobal_codebook_data; 
			pGlobal_codebook_data = nullptr;

			return false;
		}

		if (total_pvrtc_nonpow2_warnings)
		{
			LOG_WARNING("Warning: {} images could not be transcoded to PVRTC1 because one or both dimensions were not a power of 2", total_pvrtc_nonpow2_warnings);
		}

		if (total_unpack_warnings)
		{
			LOG_WARNING("ATTENTION: {} total images had invalid GPU texture data!", total_unpack_warnings);
		}
		else
		{
			LOG_INFO("Success");
		}

		delete pGlobal_codebook_data; 
		pGlobal_codebook_data = nullptr;

		for (auto data : file_data)
		{
			image_data.push_back(data);
		}
		return true;
	}

	bool Texture2D::unpackUASTC(const std::vector<uint8_t>& image_data, std::vector<uint8_t>& output_data)
	{
		// init basisu
		basisu_encoder_init();

		// trans std to basisu
		basisu::vector<uint8_t> basis_file_data;
		for (auto image : image_data)
		{
			basis_file_data.push_back(image);
		}

		// transcoder begins
		basist::basisu_transcoder dec;
		basist::basisu_file_info fileinfo;
		if (!dec.get_file_info(&basis_file_data[0], (uint32_t)basis_file_data.size(), fileinfo))
		{
			LOG_ERROR("Failed retrieving Basis file information!");
			return false;
		}

		// DEBUG - Image Info
		// assert(fileinfo.m_total_images == fileinfo.m_image_mipmap_levels.size());
		// assert(fileinfo.m_total_images == dec.get_total_images(&basis_file_data[0], (uint32_t)basis_file_data.size()));
		// printf("File info:");
		// printf("  Version: %X", fileinfo.m_version);
		// printf("  Total header size: {}", fileinfo.m_total_header_size);
		// printf("  Total selectors: {}", fileinfo.m_total_selectors);
		// printf("  Selector codebook size: {}", fileinfo.m_selector_codebook_size);
		// printf("  Total endpoints: {}", fileinfo.m_total_endpoints);
		// printf("  Endpoint codebook size: {}", fileinfo.m_endpoint_codebook_size);
		// printf("  Tables size: {}", fileinfo.m_tables_size);
		// printf("  Slices size: {}", fileinfo.m_slices_size);
		// printf("  Texture format: %s", (fileinfo.m_tex_format == basist::basis_tex_format::cUASTC4x4) ? "UASTC" : "ETC1S");
		// printf("  Texture type: %s", basist::basis_get_texture_type_name(fileinfo.m_tex_type));
		// printf("  us per frame: {} (%f fps)", fileinfo.m_us_per_frame, fileinfo.m_us_per_frame ? (1.0f / ((float)fileinfo.m_us_per_frame / 1000000.0f)) : 0.0f);
		// printf("  Total slices: {}", (uint32_t)fileinfo.m_slice_info.size());
		// printf("  Total images: %i", fileinfo.m_total_images);
		// printf("  Y Flipped: {}, Has alpha slices: {}", fileinfo.m_y_flipped, fileinfo.m_has_alpha_slices);
		// printf("  userdata0: 0x%X userdata1: 0x%X", fileinfo.m_userdata0, fileinfo.m_userdata1);
		// printf("  Per-image mipmap levels: ");
		// for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
		// 	printf("{} ", fileinfo.m_image_mipmap_levels[i]);
		// printf("");

		uint32_t total_texels = 0;
		LOG_DEBUG("Image info:");
		for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
		{
			basist::basisu_image_info ii;
			if (!dec.get_image_info(&basis_file_data[0], (uint32_t)basis_file_data.size(), ii, i))
			{
				LOG_ERROR("get_image_info() failed!");
				return false;
			}

			LOG_DEBUG("Image {}: MipLevels: {} OrigDim: {}x{}, BlockDim: {}x{}, FirstSlice: {}, HasAlpha: {}", i, ii.m_total_levels, ii.m_orig_width, ii.m_orig_height,
				ii.m_num_blocks_x, ii.m_num_blocks_y, ii.m_first_slice_index, (uint32_t)ii.m_alpha_flag);

			total_texels += ii.m_width * ii.m_height;
		}

		LOG_DEBUG("Slice info:");
		for (uint32_t i = 0; i < fileinfo.m_slice_info.size(); i++)
		{
			const basist::basisu_slice_info& sliceinfo = fileinfo.m_slice_info[i];
			LOG_DEBUG("{}: OrigWidthHeight: {}x{}, BlockDim: {}x{}, TotalBlocks: {}, Compressed size: {}, Image: {}, Level: {}, UnpackedCRC16: 0x{}, alpha: {}, iframe: {}",
				i,
				sliceinfo.m_orig_width, sliceinfo.m_orig_height,
				sliceinfo.m_num_blocks_x, sliceinfo.m_num_blocks_y,
				sliceinfo.m_total_blocks,
				sliceinfo.m_compressed_size,
				sliceinfo.m_image_index, sliceinfo.m_level_index,
				sliceinfo.m_unpacked_slice_crc16,
				(uint32_t)sliceinfo.m_alpha_flag,
				(uint32_t)sliceinfo.m_iframe_flag);
		}

		size_t comp_size = 0;
		void* pComp_data = tdefl_compress_mem_to_heap(&basis_file_data[0], basis_file_data.size(), &comp_size, TDEFL_MAX_PROBES_MASK);// TDEFL_DEFAULT_MAX_PROBES);
		mz_free(pComp_data);

		const float basis_bits_per_texel = basis_file_data.size() * 8.0f / total_texels;
		const float comp_bits_per_texel = comp_size * 8.0f / total_texels;

		LOG_DEBUG("Original size: {}, bits per texel: {}Compressed size (Deflate): {}, bits per texel: {}", (uint32_t)basis_file_data.size(), basis_bits_per_texel, (uint32_t)comp_size, comp_bits_per_texel);

		interval_timer tm;
		tm.start();

		if (!dec.start_transcoding(&basis_file_data[0], (uint32_t)basis_file_data.size()))
		{
			LOG_ERROR("start_transcoding() failed!");
			return false;
		}

		// timer
		const double start_transcoding_time_ms = tm.get_elapsed_ms();
		LOG_DEBUG("start_transcoding time: {} ms", start_transcoding_time_ms);

		image out_image;

		// Upack UASTC files separarely, to validate we can transcode slices to UASTC and unpack them to pixels.
		// This is a special path because UASTC is not yet a valid transcoder_texture_format, but a lower-level block_format.
		if (fileinfo.m_tex_format == basist::basis_tex_format::cUASTC4x4)
		{
			for (uint32_t image_index = 0; image_index < fileinfo.m_total_images; image_index++)
			{
				for (uint32_t level_index = 0; level_index < fileinfo.m_image_mipmap_levels[image_index]; level_index++)
				{
					basist::basisu_image_level_info level_info;

					if (!dec.get_image_level_info(&basis_file_data[0], (uint32_t)basis_file_data.size(), level_info, image_index, level_index))
					{
						error_printf("Failed retrieving image level information ({} {})!", image_index, level_index);
						return false;
					}

					gpu_image gpu_texture_image;
					gpu_texture_image.init(basisu::texture_format::cUASTC4x4, level_info.m_orig_width, level_info.m_orig_height);

					// Fill the buffer with psuedo-random bytes, to help more visibly detect cases where the transcoder fails to write to part of the output.
					fill_buffer_with_random_bytes(gpu_texture_image.get_ptr(), gpu_texture_image.get_size_in_bytes());

					tm.start();

					if (!dec.transcode_slice(
						&basis_file_data[0], (uint32_t)basis_file_data.size(), 
						level_info.m_first_slice_index, gpu_texture_image.get_ptr(), gpu_texture_image.get_total_blocks(), basist::block_format::cUASTC_4x4, gpu_texture_image.get_bytes_per_block()))
					{
						error_printf("Failed transcoding image level ({} {}) to UASTC!", image_index, level_index);
						return false;
					}

					double total_transcode_time = tm.get_elapsed_ms();

					LOG_DEBUG("Transcode of image {} level {} res {}x{} format UASTC_4x4 succeeded in {} ms", image_index, level_index, level_info.m_orig_width, level_info.m_orig_height, total_transcode_time);

					// unpack gpu_image to image
					image img;
					if (!gpu_texture_image.unpack(img))
					{
						error_printf("Warning: Failed unpacking GPU texture data ({} {}) to UASTC. ", image_index, level_index);
						return false;
					}
					//u.crop(level_info.m_orig_width, level_info.m_orig_height);

					// DEBUG - write out the image to a PNG file
					// std::string base_filename = "test";
					// std::string rgb_filename;
					// if (fileinfo.m_image_mipmap_levels[image_index] > 1)
					// 	rgb_filename = base_filename + string_format("_unpacked_rgb_UASTC_4x4_{}_%04u.png", level_index, image_index);
					// else
					// 	rgb_filename = base_filename + string_format("_unpacked_rgb_UASTC_4x4_%04u.png", image_index);
					//
					// if (!save_png(rgb_filename, img, cImageSaveIgnoreAlpha))
					// {
					// 	error_printf("Failed writing to PNG file \"%s\"", rgb_filename.c_str());
					// 	// delete pGlobal_codebook_data; pGlobal_codebook_data = nullptr;
					// 	return false;
					// }
					// printf("Wrote PNG file \"%s\"", rgb_filename.c_str());
					//
					// std::string alpha_filename;
					// if (fileinfo.m_image_mipmap_levels[image_index] > 1)
					// 	alpha_filename = base_filename + string_format("_unpacked_a_UASTC_4x4_{}_%04u.png", level_index, image_index);
					// else
					// 	alpha_filename = base_filename + string_format("_unpacked_a_UASTC_4x4_%04u.png", image_index);
					// if (!save_png(alpha_filename, img, cImageSaveGrayscale, 3))
					// {
					// 	error_printf("Failed writing to PNG file \"%s\"", rgb_filename.c_str());
					// 	// delete pGlobal_codebook_data; pGlobal_codebook_data = nullptr;
					// 	return false;
					// }
					// printf("Wrote PNG file \"%s\"", alpha_filename.c_str());

					out_image = img;

					// copy image to output_data
					uint32_t image_size = out_image.get_total_pixels() * 4;
					output_data.resize(image_size);
					memcpy(output_data.data(), out_image.get_ptr(), image_size);
					out_image.clear();
					img.clear();
					return true;
				}
			}
		}
		return false;
	}
}
