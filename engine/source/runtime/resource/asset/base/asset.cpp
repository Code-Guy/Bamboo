#include "asset.h"
#include "runtime/core/base/macro.h"
#include "runtime/resource/asset/asset_manager.h"

// include all asset classes
#include "runtime/resource/asset/texture_2d.h"
#include "runtime/resource/asset/texture_cube.h"
#include "runtime/resource/asset/material.h"
#include "runtime/resource/asset/static_mesh.h"
#include "runtime/resource/asset/skeletal_mesh.h"
#include "runtime/resource/asset/skeleton.h"
#include "runtime/resource/asset/animation.h"
#include "runtime/function/framework/world/world.h"

namespace Bamboo
{
	void Asset::setURL(const URL& url)
	{
		m_url = url;
		m_name = g_runtime_context.fileSystem()->basename(m_url);
		m_asset_type = g_runtime_context.assetManager()->getAssetType(m_url);
	}

	std::string Asset::getBareName()
	{
		std::string::size_type underline_pos = m_name.find_first_of('_');
		return m_name.substr(underline_pos + 1, m_name.length() - (underline_pos + 1));
	}

	std::string Asset::getFolder()
	{
		return g_runtime_context.fileSystem()->dir(m_url);
	}

}