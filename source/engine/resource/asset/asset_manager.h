#pragma once

#include "engine/resource/asset/base/asset.h"
#include "importer/import_option.h"

#define DEFAULT_MATERIAL_URL "asset/engine/material/mat_default.mat"
#define DEFAULT_TEXTURE_2D_URL "asset/engine/material/tex_dummy.tex"
#define DEFAULT_TEXTURE_CUBE_URL "asset/engine/texture/ibl/texc_cloudy.texc"
#define BRDF_TEXTURE_URL "asset/engine/texture/ibl/tex_brdf_lut.tex"

namespace Bamboo
{
	enum class EArchiveType
	{
		Json, Binary
	};

	class AssetManager
	{
	public:
		void init();
		void destroy();

		bool importGltf(const std::string& filename, const URL& folder, const GltfImportOption& option);
		bool importTexture2D(const std::string& filename, const URL& folder);
		bool importTextureCube(const std::string& filename, const URL& folder);

		bool isGltfFile(const std::string& filename);
		bool isTexture2DFile(const std::string& filename);
		bool isTextureCubeFile(const std::string& filename);

		EAssetType getAssetType(const URL& url);

		template<typename AssetClass>
		std::shared_ptr<AssetClass> loadAsset(const URL& url)
		{
			std::shared_ptr<Asset> asset = deserializeAsset(url);
			return std::dynamic_pointer_cast<AssetClass>(asset);
		}

		void serializeAsset(std::shared_ptr<Asset> asset, const URL& url = "");

	private:
		friend class GltfImporter;

		std::shared_ptr<Asset> deserializeAsset(const URL& url);
		std::string getAssetName(const std::string& asset_name, EAssetType asset_type, int asset_index = 0, const std::string& basename = "");

		std::map<URL, std::shared_ptr<Asset>> m_assets;
		std::map<EAssetType, std::string> m_asset_type_exts;
		std::map<EAssetType, EArchiveType> m_asset_archive_types;
		std::map<std::string, EAssetType> m_ext_asset_types;
	};
}