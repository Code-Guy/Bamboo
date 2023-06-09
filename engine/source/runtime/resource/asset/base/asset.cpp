#include "asset.h"
#include "runtime/core/base/macro.h"
#include "runtime/resource/asset/asset_manager.h"

namespace Bamboo
{
	void Asset::setURL(const URL& url)
	{
		m_url = url;
		m_name = g_runtime_context.fileSystem()->basename(m_url);
		m_asset_type = g_runtime_context.assetManager()->getAssetType(m_url);
	}

}