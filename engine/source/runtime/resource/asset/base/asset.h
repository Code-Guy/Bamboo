#pragma once

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/binary.hpp>

#include <rttr/registration>
#include <rttr/registration_friend.h>

#include "runtime/resource/serialization/serialization.h"

#define INVALID_INDEX -1

namespace Bamboo
{
    using URL = std::string;

	enum class EAssetType
	{
		Invalid, Texture2D, TextureCube, Material, Skeleton, StaticMesh, SkeletalMesh, Animation, World, Font
	};

	class IAssetRef
	{
	public:
		// store the reference map: property_name -> asset url
		std::map<std::string, URL> m_ref_urls;

	protected:
		virtual void bindRefs() = 0;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("ref_urls", m_ref_urls));

			bindRefs();
		}
	};

    class Asset
    {
    public:
		void setURL(const URL& url);
		const URL& getURL() { return m_url; }
		const std::string& getName() { return m_name; }
		EAssetType getAssetType() { return m_asset_type; }

		virtual void inflate() {}

    protected:
        URL m_url;
		std::string m_name;
        EAssetType m_asset_type;

    private:

    };
}