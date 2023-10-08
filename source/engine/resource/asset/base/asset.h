#pragma once

#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

#include <rttr/registration>
#include <rttr/registration_friend.h>

#include "engine/resource/serialization/serialization.h"

#define INVALID_INDEX -1

#define REF_ASSET(prop, asset) \
	prop = asset; \
	m_ref_urls[#prop] = asset->getURL();

#define REF_ASSET_ELEM(prop, id, asset) \
	prop = asset; \
	m_ref_urls[id] = asset->getURL();

#define REF_ASSET_OUTER(object, prop, asset) \
	object->##prop = asset; \
	object->m_ref_urls[#prop] = asset->getURL();

#define BIND_ASSET(prop, asset_class) \
	if (m_ref_urls.find(#prop) != m_ref_urls.end()) \
	{ \
		prop = g_engine.assetManager()->loadAsset<##asset_class>(m_ref_urls[#prop]); \
	} \

#define BIND_ASSET_ELEM(prop, id, asset_class) \
	if (m_ref_urls.find(id) != m_ref_urls.end()) \
	{ \
		prop = g_engine.assetManager()->loadAsset<##asset_class>(m_ref_urls[id]); \
	} \

namespace Bamboo
{
    using URL = std::string;

	enum class EAssetType
	{
		Invalid, Texture2D, TextureCube, Material, Skeleton, StaticMesh, SkeletalMesh, Animation, World
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

			if (!m_has_bound)
			{
				bindRefs();
			}
			m_has_bound = true;
		}

		bool m_has_bound = false;
	};

    class Asset
    {
    public:
		void setURL(const URL& url);
		void setName(const std::string& name) { m_name = name; }

		const URL& getURL() { return m_url; }
		const std::string& getName() { return m_name; }
		std::string getBareName();
		std::string getFolder();

		EAssetType getAssetType() { return m_asset_type; }		

		virtual void inflate() {}

    protected:
        URL m_url;
		std::string m_name;
        EAssetType m_asset_type;

    private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{

		}
    };
}