#include "asset_manager.h"
#include "runtime/core/base/macro.h"
#include <tinygltf/tiny_gltf.h>
#include <ktx/ktx.h>

namespace Bamboo
{
	bool AssetManager::loadModel(const std::string& url)
	{
		tinygltf::Model gltf_model;
		tinygltf::TinyGLTF gltf_context;

		std::string error;
		std::string warning;

		std::string extension = g_runtime_context.fileSystem()->extension(url);
		bool is_binary = extension == "glb";
		bool success = false;
		if (is_binary)
		{
			success = gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning, REDIRECT(url));
		}
		else
		{
			success = gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning, REDIRECT(url));
		}

		if (!success)
		{
			LOG_FATAL("failed to load gltf file {}! error: {}, warning: {}", url, error, warning);
			return false;
		}

		if (!error.empty())
		{
			LOG_ERROR("load gltf file {} error: {}", url, error);
		}
		if (!warning.empty())
		{
			LOG_WARNING("load gltf file {} warning: {}", url, warning);
		}

		return true;
	}

}