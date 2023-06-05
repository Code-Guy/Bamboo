#pragma once

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/base_class.hpp>
#include <rttr/registration>

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

    class Asset
    {
    public:
		Asset() = default;
        Asset(const URL& url);

        virtual ~Asset() = default;

		const URL& getURL() { return m_url; }
		EAssetType getAssetType() { return m_asset_type; }
		EArchiveType getArchiveType() { return m_archive_type; }

		// store the reference map: property_name -> asset url
		std::map<std::string, URL> m_ref_urls;

    protected:
        virtual void inflate();

        URL m_url;
        EAssetType m_asset_type;
        EArchiveType m_archive_type;

    private:
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(m_ref_urls);
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);
		}
    };
}

// register all user defined serialization
namespace cereal
{
	template <class Archive>
	void save(Archive& ar, const glm::vec4& v4)
	{
		ar(v4.x, v4.y, v4.z, v4.w);
	}

	template <class Archive>
	void load(Archive& ar, glm::vec4& v4)
	{
		ar(v4.x, v4.y, v4.z, v4.w);
	}
}