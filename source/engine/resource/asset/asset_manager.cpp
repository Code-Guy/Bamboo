#include "asset_manager.h"

#include "engine/resource/asset/texture_2d.h"
#include "engine/resource/asset/texture_cube.h"
#include "engine/function/framework/world/world.h"

#include "importer/gltf_importer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <tinygltf/stb_image.h>

#include <fstream>
#include <basisu/encoder/basisu_comp.h>

namespace Bamboo
{
	void AssetManager::init()
	{
		m_asset_type_exts = {
			{ EAssetType::Texture2D, "tex" },
			{ EAssetType::Texture2D_Ktx, "ktx" },
			{ EAssetType::TextureCube, "texc" }, 
			{ EAssetType::Material, "mat" }, 
			{ EAssetType::Skeleton, "skl" },
			{ EAssetType::StaticMesh, "sm"}, 
			{ EAssetType::SkeletalMesh, "skm" }, 
			{ EAssetType::Animation, "anim" },
			{ EAssetType::World, "world" }
		};

		m_asset_archive_types = {
			{ EAssetType::Texture2D, EArchiveType::Binary },
			{ EAssetType::Texture2D_Ktx, EArchiveType::Binary },
			{ EAssetType::TextureCube, EArchiveType::Binary },
			{ EAssetType::Material, EArchiveType::Json },
			{ EAssetType::Skeleton, EArchiveType::Binary },
			{ EAssetType::StaticMesh, EArchiveType::Binary },
			{ EAssetType::SkeletalMesh, EArchiveType::Binary },
			{ EAssetType::Animation, EArchiveType::Binary },
			{ EAssetType::World, EArchiveType::Json }
		};

		for (const auto& iter : m_asset_type_exts)
		{
			m_ext_asset_types[iter.second] = iter.first;
		}
	}

	void AssetManager::destroy()
	{
		for (auto& iter : m_assets)
		{
			iter.second.reset();
		}
		m_assets.clear();
	}

	bool AssetManager::importGltf(const std::string& filename, const URL& folder, const GltfImportOption& option)
	{
		return GltfImporter::importGltf(filename, folder, option);
	}

	bool AssetManager::importTexture2D(const std::string& filename, const URL& folder)
	{
		// uint32_t width, height;
		// uint32_t k_channels = 4;
		// uint8_t* image_data = stbi_load(filename.c_str(), (int*)&width, (int*)&height, 0, k_channels);
		// ASSERT(image_data != nullptr, "failed to import texture: {}", filename)

		// todo load from file
		basisu::image image;
		size_t image_size = 0;
		void* p_image_data = compressTexture2D(filename, image, image_size);
		if (p_image_data == nullptr)
		{
			printf("Compress Texture2D failed\n");
			return false;
		}

		std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
		std::string asset_name = getAssetName(filename, EAssetType::Texture2D);
		URL url = URL::combine(folder.str(), asset_name);
		texture->setURL(url);

		texture->m_width = image.get_width();
		texture->m_height = image.get_height();

		texture->m_image_data.resize(image_size);
		memcpy(texture->m_image_data.data(), p_image_data, image_size);
		basisu::basis_free_data(p_image_data);

		texture->inflate();
		serializeAsset(texture);

		return true;
	}

	bool AssetManager::importTextureCube(const std::string& filename, const URL& folder)
	{
		std::shared_ptr<TextureCube> texture_cube = std::make_shared<TextureCube>();
		std::string asset_name = getAssetName(filename, EAssetType::TextureCube);
		URL url = URL::combine(folder.str(), asset_name);
		texture_cube->setURL(url);

		texture_cube->setAddressMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		texture_cube->m_texture_type = ETextureType::Cube;
		texture_cube->m_pixel_type = EPixelType::RGBA16;
		g_engine.fileSystem()->loadBinary(filename, texture_cube->m_image_data);

		texture_cube->inflate();
		serializeAsset(texture_cube);

		return true;
	}

	bool AssetManager::isGltfFile(const std::string& filename)
	{
		std::string extension = g_engine.fileSystem()->extension(filename);
		return extension == "glb" || extension == "gltf";
	}

	bool AssetManager::isTexture2DFile(const std::string& filename)
	{
		std::string extension = g_engine.fileSystem()->extension(filename);
		return extension == "png" || extension == "jpg";
	}

	bool AssetManager::isTextureCubeFile(const std::string& filename)
	{
		std::string extension = g_engine.fileSystem()->extension(filename);
		return extension == "ktx";
	}

	EAssetType AssetManager::getAssetType(const URL& url)
	{
		std::string extension = g_engine.fileSystem()->extension(url.str());
		if (m_ext_asset_types.find(extension) != m_ext_asset_types.end())
		{
			return m_ext_asset_types[extension];
		}
		return EAssetType::Invalid;
	}

	void AssetManager::serializeAsset(std::shared_ptr<Asset> asset, const URL& url)
	{
		// reference asset
		EAssetType asset_type = asset->getAssetType();
		const std::string& asset_ext = m_asset_type_exts[asset_type];
		EArchiveType archive_type = m_asset_archive_types[asset_type];
		std::string filename = url.empty() ? asset->getURL().getAbsolute() : url.getAbsolute();

		switch (archive_type)
		{
		case EArchiveType::Json:
		{
			std::ofstream ofs(filename);
			cereal::JSONOutputArchive archive(ofs);
			archive(cereal::make_nvp(asset_ext.c_str(), asset));
		}
		break;
		case EArchiveType::Binary:
		{
			std::ofstream ofs(filename, std::ios::binary);
			cereal::BinaryOutputArchive archive(ofs);
			archive(cereal::make_nvp(asset_ext.c_str(), asset));
		}
		break;
		default:
			break;
		}

		// don't cache world!
		if (asset_type != EAssetType::World)
		{
			m_assets[url] = asset;
		}
	}

	std::shared_ptr<Asset> AssetManager::deserializeAsset(const URL& url)
	{
		// check if the asset url exists
		if (!g_engine.fileSystem()->exists(url.str()))
		{
			return nullptr;
		}

		// check if the asset has been loaded
		if (m_assets.find(url) != m_assets.end())
		{
			return m_assets[url];
		}

		EAssetType asset_type = getAssetType(url);
		EArchiveType archive_type = m_asset_archive_types[asset_type];
		const std::string& asset_ext = m_asset_type_exts[asset_type];
		std::string filename = url.getAbsolute();
		std::shared_ptr<Asset> asset = nullptr;

		switch (archive_type)
		{
		case EArchiveType::Json:
		{
			std::ifstream ifs(filename);
			cereal::JSONInputArchive archive(ifs);
			archive(asset);
		}
		break;
		case EArchiveType::Binary:
		{
			std::ifstream ifs(filename, std::ios::binary);
			cereal::BinaryInputArchive archive(ifs);
			archive(asset);
		}
		break;
		default:
			break;
		}

		asset->setURL(url);
		asset->inflate();

		// don't cache world!
		if (asset_type != EAssetType::World)
		{
			m_assets[url] = asset;
		}
		
		return asset;
	}

	std::string AssetManager::getAssetName(const std::string& asset_name, EAssetType asset_type, int asset_index, const std::string& basename)
	{
		const std::string& ext = m_asset_type_exts[asset_type];
		if (!asset_name.empty())
		{
			std::string asset_basename = g_engine.fileSystem()->basename(asset_name);
			asset_basename = g_engine.fileSystem()->validateBasename(asset_basename);
			return g_engine.fileSystem()->format("%s_%s.%s", ext.c_str(), asset_basename.c_str(), ext.c_str());
		}
		return g_engine.fileSystem()->format("%s_%s_%d.%s", ext.c_str(), basename.c_str(), asset_index, ext.c_str());
	}

	void* AssetManager::compressTexture2D(const std::string& filename, basisu::image& source_image, size_t& data_size)
	{
		basisu::basisu_encoder_init();

		basisu::vector<basisu::image> source_images(1);

		//source_image = source_images[0];
		if (!load_image(filename.c_str(), source_image))
		{
			basisu::error_printf("Failed loading test image \"%s\"\n", filename.c_str());
			return nullptr;
		}

		printf("Loaded file \"%s\", dimemsions %ux%u has alpha: %u\n", filename.c_str(), source_image.get_width(), source_image.get_height(), source_image.has_alpha());

		basisu::image_stats stats;

		uint32_t flags_and_quality;
		float uastc_rdo_quality = 1.0f;

		// UASTC
		flags_and_quality = basisu::cFlagThreaded | basisu::cFlagUASTCRDO | basisu::cFlagPrintStats | basisu::cFlagPrintStatus;

		source_images[0] = source_image;
		void* p_image_data = nullptr;
		p_image_data = basis_compress(source_images, flags_and_quality, uastc_rdo_quality, &data_size, &stats);
		if (!p_image_data)
		{
			basisu::error_printf("basis_compress() failed!\n");
			return nullptr;
		}

		printf("UASTC Size: %u, PSNR: %f\n", (uint32_t)data_size, stats.m_basis_rgba_avg_psnr);
		return p_image_data;
	}
}
