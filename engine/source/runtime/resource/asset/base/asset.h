#pragma once

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/base_class.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define INVALID_INDEX -1

namespace Bamboo
{
    using URL = std::string;

	enum class EAssetType
	{
		Invalid, Texture2D, TextureCube, Material, Skeleton, StaticMesh, SkeletalMesh, Animation
	};

	enum class EArchiveType
	{
		Json, Binary
	};

    struct RefAsset
    {
        URL url;
        std::shared_ptr<class Asset> asset;
    };

    class Asset
    {
    public:
        Asset(const URL& url);

        virtual ~Asset() = default;

		const URL& getURL() { return m_url; }
		EAssetType getAssetType() { return m_asset_type; }
		EArchiveType getArchiveType() { return m_archive_type; }

    protected:
        virtual void inflate() {}

        URL m_url;
        EAssetType m_asset_type;
        EArchiveType m_archive_type;
        std::vector<RefAsset> m_ref_assets;

    private:
		friend class cereal::access;
		template<class Archive>
		void save(Archive& ar) const
		{

		}

		template<class Archive>
		void load(Archive& ar)
		{

		}
    };
}