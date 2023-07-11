#include "asset_manager.h"

#include "runtime/resource/asset/texture_2d.h"
#include "runtime/function/framework/world/world.h"

#include "importer/gltf_importer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "tinygltf/stb_image.h"

#include <fstream>

namespace Bamboo
{
	void AssetManager::init()
	{
		m_asset_type_exts = {
			{ EAssetType::Texture2D, "tex" },
			{ EAssetType::TextureCube, "texc" }, 
			{ EAssetType::Material, "mat" }, 
			{ EAssetType::Skeleton, "skl" },
			{ EAssetType::StaticMesh, "sm"}, 
			{ EAssetType::SkeletalMesh, "skm" }, 
			{ EAssetType::Animation, "anim" },
			{ EAssetType::World, "world" },
			{ EAssetType::Font, "ttf" }
		};

		m_asset_archive_types = {
			{ EAssetType::Texture2D, EArchiveType::Binary },
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
		uint32_t width, height;
		uint8_t* image_data = stbi_load(filename.c_str(), (int*)&width, (int*)&height, 0, IMAGE_COMPONENT);
		ASSERT(image_data != nullptr, "failed to import texture: {}", filename);

		std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
		std::string asset_name = getAssetName(filename, EAssetType::Texture2D);
		URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
		texture->setURL(url);

		texture->m_width = width;
		texture->m_height = height;

		size_t image_size = width * height * IMAGE_COMPONENT;
		texture->m_image_data.resize(image_size);
		memcpy(texture->m_image_data.data(), image_data, image_size);

		texture->inflate();
		serializeAsset(texture);

		m_assets[url] = texture;
		return true;
	}

	bool AssetManager::isGltfFile(const std::string& filename)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		return extension == "glb" || extension == "gltf";
	}

	bool AssetManager::isTexture2DFile(const std::string& filename)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		return extension == "png" || extension == "jpg";
	}

	EAssetType AssetManager::getAssetType(const URL& url)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(url);
		if (m_ext_asset_types.find(extension) != m_ext_asset_types.end())
		{
			return m_ext_asset_types[extension];
		}
		return EAssetType::Invalid;
	}

	void AssetManager::serializeAsset(std::shared_ptr<Asset> asset)
	{
		EAssetType asset_type = asset->getAssetType();
		const std::string& asset_ext = m_asset_type_exts[asset_type];
		EArchiveType archive_type = m_asset_archive_types[asset_type];
		std::string filename = TO_ABSOLUTE(asset->getURL());

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
	}

	std::shared_ptr<Asset> AssetManager::deserializeAsset(const URL& url)
	{
		// check if the asset url exists
		if (!g_runtime_context.fileSystem()->exists(url))
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
		std::string filename = TO_ABSOLUTE(url);
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
		m_assets[url] = asset;

		return asset;
	}

	std::string AssetManager::getAssetName(const std::string& asset_name, EAssetType asset_type, int asset_index, const std::string& basename)
	{
		const std::string& ext = m_asset_type_exts[asset_type];
		if (!asset_name.empty())
		{
			std::string asset_basename = g_runtime_context.fileSystem()->basename(asset_name);
			return g_runtime_context.fileSystem()->format("%s_%s.%s", ext.c_str(), asset_basename.c_str(), ext.c_str());
		}
		return g_runtime_context.fileSystem()->format("%s_%s_%d.%s", ext.c_str(), basename.c_str(), asset_index, ext.c_str());
	}
}