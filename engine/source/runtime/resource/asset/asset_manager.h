#pragma once

#include "runtime/resource/asset/base/asset.h"
#include <unordered_map>

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

		bool importAsset(const std::string& filename, const URL& folder);
		EAssetType getAssetType(const URL& url);

		template<typename AssetClass>
		std::shared_ptr<AssetClass> loadAsset(const URL& url)
		{
			std::shared_ptr<Asset> asset = deserializeAsset(url);
			return std::dynamic_pointer_cast<AssetClass>(asset);
		}

	private:
		bool importGltf(const std::string& filename, const URL& folder, bool is_combined = false);
		void serializeAsset(std::shared_ptr<Asset> asset);
		std::shared_ptr<Asset> deserializeAsset(const URL& url);

		std::string getAssetName(const std::string& basename, const std::string& asset_name, EAssetType asset_type, int asset_index);

		std::unordered_map<URL, std::shared_ptr<Asset>> m_assets;
		std::map<EAssetType, std::string> m_asset_type_exts;
		std::map<EAssetType, EArchiveType> m_asset_archive_types;
		std::map<std::string, EAssetType> m_ext_asset_types;
	};
}