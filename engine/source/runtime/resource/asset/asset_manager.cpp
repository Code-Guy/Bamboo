#include "asset_manager.h"
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/material.h"
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>
#include <ktx/ktx.h>

namespace Bamboo
{
	void AssetManager::init()
	{
		m_asset_exts = {
			{ EAssetType::Texture2D, "tex"},
			{ EAssetType::TextureCube, "texc"}, 
			{ EAssetType::Material, "mat"}, 
			{ EAssetType::Skeleton, "skl"},
			{ EAssetType::StaticMesh, "sm"}, 
			{ EAssetType::SkeletalMesh, "skm"}, 
			{ EAssetType::Animation, "anim"}
		};
	}

	void AssetManager::destroy()
	{
		for (auto& iter : m_assets)
		{
			iter.second.reset();
		}
		m_assets.clear();
	}

	bool AssetManager::importAsset(const std::string& filename, const URL& folder)
	{
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		if (extension == "glb" || extension == "gltf")
		{
			return importGltf(filename, folder);
		}
		else
		{
			LOG_WARNING("unknown asset format");
			return false;
		}
	}

	std::shared_ptr<Asset> AssetManager::loadAssetImpl(const URL& url)
	{
		return nullptr;
	}

	bool AssetManager::importGltf(const std::string& filename, const URL& folder)
	{
		tinygltf::Model gltf_model;
		tinygltf::TinyGLTF gltf_context;

		std::string error;
		std::string warning;

		std::string basename = g_runtime_context.fileSystem()->basename(filename);
		std::string extension = g_runtime_context.fileSystem()->extension(filename);
		bool is_binary = extension == "glb";
		bool success = false;
		if (is_binary)
		{
			success = gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning, filename);
		}
		else
		{
			success = gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, filename);
		}

		if (!success)
		{
			LOG_FATAL("failed to load gltf file {}! error: {}, warning: {}", filename, error, warning);
			return false;
		}

		if (!error.empty())
		{
			LOG_ERROR("load gltf file {} error: {}", filename, error);
		}
		if (!warning.empty())
		{
			LOG_WARNING("load gltf file {} warning: {}", filename, warning);
		}

		std::map<EAssetType, int> asset_indices = {
			{ EAssetType::Texture2D, 0},
			{ EAssetType::TextureCube, 0},
			{ EAssetType::Material, 0},
			{ EAssetType::Skeleton, 0},
			{ EAssetType::StaticMesh, 0},
			{ EAssetType::SkeletalMesh, 0},
			{ EAssetType::Animation, 0}
		};

		// 1.load textures and samplers
		std::vector<std::shared_ptr<Texture2D>> textures;
		for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
		{
			const tinygltf::Image& gltf_image = gltf_model.images[gltf_texture.source];
			const tinygltf::Sampler& gltf_sampler = gltf_model.samplers[gltf_texture.sampler];

			EAssetType asset_type = EAssetType::Texture2D;
			std::string asset_name = getAssetName(basename, gltf_texture.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);
			std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(url);
			texture->loadFromGltf(gltf_image, gltf_sampler);
			serializeAsset(texture);

			textures.push_back(texture);
			m_assets[url] = texture;
		}

		// 2.load materials
		for (const tinygltf::Material& gltf_material : gltf_model.materials)
		{
			EAssetType asset_type = EAssetType::Material;
			std::string asset_name = getAssetName(basename, gltf_material.name, asset_type, asset_indices[asset_type]++);
			URL url = g_runtime_context.fileSystem()->combine(folder, asset_name);

			std::shared_ptr<Material> material = std::make_shared<Material>(url);

			material->m_base_color_factor = glm::make_vec4(gltf_material.pbrMetallicRoughness.baseColorFactor.data());
			material->m_emissive_factor = glm::make_vec4(gltf_material.emissiveFactor.data());
			material->m_metallic_factor = gltf_material.pbrMetallicRoughness.metallicFactor;
			material->m_roughness_factor = gltf_material.pbrMetallicRoughness.roughnessFactor;

			if (gltf_material.pbrMetallicRoughness.baseColorTexture.index != INVALID_INDEX)
			{
				material->m_base_color_texure = textures[gltf_material.pbrMetallicRoughness.baseColorTexture.index];
				material->m_base_color_texure->setTextureType(TextureType::BaseColor);
				ASSERT(gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index != INVALID_INDEX)
			{
				material->m_metallic_roughness_texure = textures[gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index];
				material->m_metallic_roughness_texure->setTextureType(TextureType::MetallicRoughness);
				ASSERT(gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.normalTexture.index != INVALID_INDEX)
			{
				material->m_normal_texure = textures[gltf_material.normalTexture.index];
				material->m_normal_texure->setTextureType(TextureType::Normal);
				ASSERT(gltf_material.normalTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.occlusionTexture.index != INVALID_INDEX)
			{
				material->m_occlusion_texure = textures[gltf_material.occlusionTexture.index];
				material->m_occlusion_texure->setTextureType(TextureType::Occlusion);
				ASSERT(gltf_material.occlusionTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}
			if (gltf_material.emissiveTexture.index != INVALID_INDEX)
			{
				material->m_emissive_texure = textures[gltf_material.emissiveTexture.index];
				material->m_emissive_texure->setTextureType(TextureType::Emissive);
				ASSERT(gltf_material.emissiveTexture.texCoord == 0, "do not support non-zero texture coordinate index");
			}

			m_assets[url] = material;
		}

		return true;
	}

	std::string AssetManager::getAssetName(const std::string& basename, const std::string& asset_name, EAssetType asset_type, int asset_index)
	{
		const std::string& ext = m_asset_exts[asset_type];
		if (!asset_name.empty())
		{
			std::string asset_basename = g_runtime_context.fileSystem()->basename(asset_name);
			return g_runtime_context.fileSystem()->format("%s_%s.%s", ext.c_str(), asset_basename.c_str(), ext.c_str());
		}
		return g_runtime_context.fileSystem()->format("%s_%s_%d.%s", ext.c_str(), basename.c_str(), asset_index, ext.c_str());
	}

	void AssetManager::serializeAsset(std::shared_ptr<Asset> asset)
	{
		std::string filename = TO_ABSOLUTE(asset->getURL());
		std::ofstream ofs(filename);

#define ARCHIVE_ASSET(type, asset) \
	case EAssetType:: ##type: \
		archive(std::dynamic_pointer_cast<##type>(asset)); \
	break

		switch (asset->getArchiveType())
		{
		case EArchiveType::Json:
		{
			cereal::JSONOutputArchive archive(ofs);

			switch (asset->getAssetType())
			{
				ARCHIVE_ASSET(Texture2D, asset);
			default:
				break;
			}
		}
		break;
		case EArchiveType::Binary:
		{
			cereal::BinaryOutputArchive archive(ofs);
			
			switch (asset->getAssetType())
			{
				ARCHIVE_ASSET(Texture2D, asset);
			default:
				break;
			}
		}
		break;
		default:
			break;
		}

#undef ARCHIVE_ASSET
	}

	void AssetManager::deserializeAsset(std::shared_ptr<Asset> asset)
	{

	}
}