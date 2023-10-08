#include "asset.h"
#include "engine/core/base/macro.h"
#include "engine/resource/asset/asset_manager.h"

// include all asset classes
#include "engine/resource/asset/texture_2d.h"
#include "engine/resource/asset/texture_cube.h"
#include "engine/resource/asset/material.h"
#include "engine/resource/asset/static_mesh.h"
#include "engine/resource/asset/skeletal_mesh.h"
#include "engine/resource/asset/skeleton.h"
#include "engine/resource/asset/animation.h"
#include "engine/function/framework/world/world.h"

namespace Bamboo
{
	void Asset::setURL(const URL& url)
	{
		m_url = url;
		m_name = g_engine.fileSystem()->basename(m_url);
		m_asset_type = g_engine.assetManager()->getAssetType(m_url);
	}

	std::string Asset::getBareName()
	{
		std::string::size_type underline_pos = m_name.find_first_of('_');
		return m_name.substr(underline_pos + 1, m_name.length() - (underline_pos + 1));
	}

	std::string Asset::getFolder()
	{
		return g_engine.fileSystem()->dir(m_url);
	}

}