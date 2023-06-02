#include "asset_manager.h"
#include "runtime/resource/asset/texture_2d.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tinygltf/tiny_gltf.h>
#include <ktx/ktx.h>

namespace Bamboo
{
	void AssetManager::init()
	{
		
	}

	void AssetManager::destroy()
	{

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
			LOG_FATAL("unknown import asset format");
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

		// 1.load textures and samplers
		for (const tinygltf::Texture& gltf_texture : gltf_model.textures)
		{
			const tinygltf::Image& gltf_image = gltf_model.images[gltf_texture.source];
			const tinygltf::Sampler& gltf_sampler = gltf_model.samplers[gltf_texture.sampler];

			std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>();
			texture->loadFromGltf(gltf_image, gltf_sampler);
			
		}

		return true;
	}

}