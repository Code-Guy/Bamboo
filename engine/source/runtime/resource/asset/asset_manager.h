#pragma once

#include "runtime/resource/asset/base/asset.h"
#include <unordered_map>

namespace Bamboo
{
	class AssetManager
	{
	public:
		void init();
		void destroy();

		bool importAsset(const std::string& filename, const URL& folder);

		template<typename AssetClass>
		std::shared_ptr<AssetClass> loadAsset(const URL& url)
		{
			std::shared_ptr<Asset> asset = loadAssetImpl(url);
			return std::dynamic_pointer_cast<AssetClass>(asset);
		}

	private:
		std::shared_ptr<Asset> loadAssetImpl(const URL& url);
		bool importGltf(const std::string& filename, const URL& folder, bool is_combined = false);

		std::string getAssetName(const std::string& basename, const std::string& asset_name, EAssetType asset_type, int asset_index);

		void serializeAsset(std::shared_ptr<Asset> asset);
		void deserializeAsset(std::shared_ptr<Asset> asset);

		std::unordered_map<URL, std::shared_ptr<Asset>> m_assets;
		std::unordered_map<URL, std::vector<URL>> m_refs;
		std::unordered_map<URL, std::vector<URL>> m_inv_refs;

		std::map<EAssetType, std::string> m_asset_exts;
	};
}