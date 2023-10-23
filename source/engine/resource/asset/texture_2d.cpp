#include "texture_2d.h"
#include "basisu/basisu_tool.cpp"

CEREAL_REGISTER_TYPE(Bamboo::Texture2D)

CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Asset, Bamboo::Texture2D)
//
// #define KTX_CHECK(x)                                                                              \
// 	do                                                                                            \
// 	{                                                                                             \
// 		KTX_error_code err = x;                                                                   \
// 		if (err != KTX_SUCCESS)                                                                   \
// 		{                                                                                         \
// 			auto index = static_cast<uint32_t>(err);                                              \
// 			LOGE("Detected KTX error: {}", index < error_codes.size() ? error_codes[index] : ""); \
// 			abort();                                                                              \
// 		}                                                                                         \
// 	} while (0)

namespace Bamboo
{
	void Texture2D::inflate()
	{
		m_layers = 1;
		m_mip_levels = m_texture_type == ETextureType::BaseColor ? VulkanUtil::calcMipLevel(m_width, m_height) : 1;

		std::vector<uint8_t> unpacked_image;
		if (!unpackUASTC(m_image_data, unpacked_image))
		// if (!unpackKTX(m_image_data))
		{
			printf("Unpack KTX failed\n");
			return;
		}
		VulkanUtil::createImageViewSampler(m_width, m_height, unpacked_image.data(), m_mip_levels, m_layers, getFormat(), m_min_filter, m_mag_filter, m_address_mode_u, m_image_view_sampler);
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
			printf("Warning: %u images could not be transcoded to PVRTC1 because one or both dimensions were not a power of 2\n", total_pvrtc_nonpow2_warnings);
		}

		if (total_unpack_warnings)
		{
			printf("ATTENTION: %u total images had invalid GPU texture data!\n", total_unpack_warnings);
		}
		else
		{
			printf("Success\n");
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
		//image_data.clear();

		// read raw file?
		// basis_data* pGlobal_codebook_data = nullptr;
		// if (opts.m_etc1s_use_global_codebooks_file.size())
		// {
		// 	pGlobal_codebook_data = load_basis_file(opts.m_etc1s_use_global_codebooks_file.c_str(), true);
		// 	if (!pGlobal_codebook_data)
		// 	{
		// 		error_printf("Failed loading global codebook data from file \"%s\"\n", opts.m_etc1s_use_global_codebooks_file.c_str());
		// 		return false;
		// 	}
		// 	printf("Loaded global codebooks from file \"%s\"\n", opts.m_etc1s_use_global_codebooks_file.c_str());
		// }

		// transcoder begins
		basist::basisu_transcoder dec;
		basist::basisu_file_info fileinfo;
		if (!dec.get_file_info(&basis_file_data[0], (uint32_t)basis_file_data.size(), fileinfo))
		{
			error_printf("Failed retrieving Basis file information!\n");
			return false;
		}

		// debug
		assert(fileinfo.m_total_images == fileinfo.m_image_mipmap_levels.size());
		assert(fileinfo.m_total_images == dec.get_total_images(&basis_file_data[0], (uint32_t)basis_file_data.size()));
		printf("File info:\n");
		printf("  Version: %X\n", fileinfo.m_version);
		printf("  Total header size: %u\n", fileinfo.m_total_header_size);
		printf("  Total selectors: %u\n", fileinfo.m_total_selectors);
		printf("  Selector codebook size: %u\n", fileinfo.m_selector_codebook_size);
		printf("  Total endpoints: %u\n", fileinfo.m_total_endpoints);
		printf("  Endpoint codebook size: %u\n", fileinfo.m_endpoint_codebook_size);
		printf("  Tables size: %u\n", fileinfo.m_tables_size);
		printf("  Slices size: %u\n", fileinfo.m_slices_size);
		printf("  Texture format: %s\n", (fileinfo.m_tex_format == basist::basis_tex_format::cUASTC4x4) ? "UASTC" : "ETC1S");
		printf("  Texture type: %s\n", basist::basis_get_texture_type_name(fileinfo.m_tex_type));
		printf("  us per frame: %u (%f fps)\n", fileinfo.m_us_per_frame, fileinfo.m_us_per_frame ? (1.0f / ((float)fileinfo.m_us_per_frame / 1000000.0f)) : 0.0f);
		printf("  Total slices: %u\n", (uint32_t)fileinfo.m_slice_info.size());
		printf("  Total images: %i\n", fileinfo.m_total_images);
		printf("  Y Flipped: %u, Has alpha slices: %u\n", fileinfo.m_y_flipped, fileinfo.m_has_alpha_slices);
		printf("  userdata0: 0x%X userdata1: 0x%X\n", fileinfo.m_userdata0, fileinfo.m_userdata1);
		printf("  Per-image mipmap levels: ");
		for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
			printf("%u ", fileinfo.m_image_mipmap_levels[i]);
		printf("\n");

		uint32_t total_texels = 0;
		printf("\nImage info:\n");
		for (uint32_t i = 0; i < fileinfo.m_total_images; i++)
		{
			basist::basisu_image_info ii;
			if (!dec.get_image_info(&basis_file_data[0], (uint32_t)basis_file_data.size(), ii, i))
			{
				error_printf("get_image_info() failed!\n");
				return false;
			}

			printf("Image %u: MipLevels: %u OrigDim: %ux%u, BlockDim: %ux%u, FirstSlice: %u, HasAlpha: %u\n", i, ii.m_total_levels, ii.m_orig_width, ii.m_orig_height,
				ii.m_num_blocks_x, ii.m_num_blocks_y, ii.m_first_slice_index, (uint32_t)ii.m_alpha_flag);

			total_texels += ii.m_width * ii.m_height;
		}

		printf("\nSlice info:\n");
		for (uint32_t i = 0; i < fileinfo.m_slice_info.size(); i++)
		{
			const basist::basisu_slice_info& sliceinfo = fileinfo.m_slice_info[i];
			printf("%u: OrigWidthHeight: %ux%u, BlockDim: %ux%u, TotalBlocks: %u, Compressed size: %u, Image: %u, Level: %u, UnpackedCRC16: 0x%X, alpha: %u, iframe: %i\n",
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
		printf("\n");

		size_t comp_size = 0;
		void* pComp_data = tdefl_compress_mem_to_heap(&basis_file_data[0], basis_file_data.size(), &comp_size, TDEFL_MAX_PROBES_MASK);// TDEFL_DEFAULT_MAX_PROBES);
		mz_free(pComp_data);

		const float basis_bits_per_texel = basis_file_data.size() * 8.0f / total_texels;
		const float comp_bits_per_texel = comp_size * 8.0f / total_texels;

		printf("Original size: %u, bits per texel: %3.3f\nCompressed size (Deflate): %u, bits per texel: %3.3f\n", (uint32_t)basis_file_data.size(), basis_bits_per_texel, (uint32_t)comp_size, comp_bits_per_texel);

		interval_timer tm;
		tm.start();

		if (!dec.start_transcoding(&basis_file_data[0], (uint32_t)basis_file_data.size()))
		{
			error_printf("start_transcoding() failed!\n");
			return false;
		}

		// timer
		const double start_transcoding_time_ms = tm.get_elapsed_ms();
		printf("start_transcoding time: %3.3f ms\n", start_transcoding_time_ms);

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
						error_printf("Failed retrieving image level information (%u %u)!\n", image_index, level_index);
						return false;
					}

					gpu_image gi;
					gi.init(basisu::texture_format::cUASTC4x4, level_info.m_orig_width, level_info.m_orig_height);

					// Fill the buffer with psuedo-random bytes, to help more visibly detect cases where the transcoder fails to write to part of the output.
					fill_buffer_with_random_bytes(gi.get_ptr(), gi.get_size_in_bytes());

					//uint32_t decode_flags = 0;

					tm.start();

					if (!dec.transcode_slice(
						&basis_file_data[0], (uint32_t)basis_file_data.size(), 
						level_info.m_first_slice_index, gi.get_ptr(), gi.get_total_blocks(), basist::block_format::cUASTC_4x4, gi.get_bytes_per_block()))
					{
						error_printf("Failed transcoding image level (%u %u) to UASTC!\n", image_index, level_index);
						return false;
					}

					double total_transcode_time = tm.get_elapsed_ms();

					printf("Transcode of image %u level %u res %ux%u format UASTC_4x4 succeeded in %3.3f ms\n", image_index, level_index, level_info.m_orig_width, level_info.m_orig_height, total_transcode_time);

					image u;
					if (!gi.unpack(u))
					{
						error_printf("Warning: Failed unpacking GPU texture data (%u %u) to UASTC. \n", image_index, level_index);
						return false;
					}
					//u.crop(level_info.m_orig_width, level_info.m_orig_height);

					std::string base_filename = "test";
					std::string rgb_filename;
					if (fileinfo.m_image_mipmap_levels[image_index] > 1)
						rgb_filename = base_filename + string_format("_unpacked_rgb_UASTC_4x4_%u_%04u.png", level_index, image_index);
					else
						rgb_filename = base_filename + string_format("_unpacked_rgb_UASTC_4x4_%04u.png", image_index);

					if (!save_png(rgb_filename, u, cImageSaveIgnoreAlpha))
					{
						error_printf("Failed writing to PNG file \"%s\"\n", rgb_filename.c_str());
						// delete pGlobal_codebook_data; pGlobal_codebook_data = nullptr;
						return false;
					}
					printf("Wrote PNG file \"%s\"\n", rgb_filename.c_str());

					std::string alpha_filename;
					if (fileinfo.m_image_mipmap_levels[image_index] > 1)
						alpha_filename = base_filename + string_format("_unpacked_a_UASTC_4x4_%u_%04u.png", level_index, image_index);
					else
						alpha_filename = base_filename + string_format("_unpacked_a_UASTC_4x4_%04u.png", image_index);
					if (!save_png(alpha_filename, u, cImageSaveGrayscale, 3))
					{
						error_printf("Failed writing to PNG file \"%s\"\n", rgb_filename.c_str());
						// delete pGlobal_codebook_data; pGlobal_codebook_data = nullptr;
						return false;
					}
					printf("Wrote PNG file \"%s\"\n", alpha_filename.c_str());

					out_image = u;

					// size_t PNG_data_size = 0;
					// void *pPNG_data = buminiz::tdefl_write_image_to_png_file_in_memory_ex(u.get_ptr(), u.get_width(), u.get_height(), 4, &PNG_data_size, 1, false);
					// image_data.resize(PNG_data_size);
					// memcpy(image_data.data(), pPNG_data, PNG_data_size);
					// free(pPNG_data);
					uint32_t image_size = out_image.get_total_pixels() * 4;
					output_data.resize(image_size);
					memcpy(output_data.data(), out_image.get_ptr(), image_size);
					out_image.clear();
					u.clear();
					return true;
				}
			}
		}
		// uint32_t image_size = out_image.get_total_pixels() * 4;
		// image_data.resize(image_size);
		// memcpy(image_data.data(), out_image.get_ptr(), image_size);

		// trans to image data
		// for (auto data : basis_file_data)
		// {
		// 	image_data.push_back(data);
		// }
		return true;
	}

	bool Texture2D::unpackTexture(image img)
	{
		if (!img.get_total_pixels())
			return false;
				
		void* pPNG_data = nullptr;
		size_t PNG_data_size = 0;

		
		bool has_alpha = img.has_alpha();

		if (!has_alpha)
		{
			uint8_vec rgb_pixels(img.get_total_pixels() * 3);
			uint8_t* pDst = &rgb_pixels[0];

			for (uint32_t y = 0; y < img.get_height(); y++)
			{
				const color_rgba* pSrc = &img(0, y);
				for (uint32_t x = 0; x < img.get_width(); x++)
				{
					pDst[0] = pSrc->r;
					pDst[1] = pSrc->g;
					pDst[2] = pSrc->b;
					
					pSrc++;
					pDst += 3;
				}
			}

			//pPNG_data = buminiz::tdefl_write_image_to_png_file_in_memory_ex(rgb_pixels.data(), img.get_width(), img.get_height(), 3, &PNG_data_size, 1, false);
		}
		else
		{
			//pPNG_data = buminiz::tdefl_write_image_to_png_file_in_memory_ex(img.get_ptr(), img.get_width(), img.get_height(), 4, &PNG_data_size, 1, false);
		}

		if (!pPNG_data)
			return false;

		free(pPNG_data);

		return true;
	}
}
