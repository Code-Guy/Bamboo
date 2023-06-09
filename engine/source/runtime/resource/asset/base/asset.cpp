#include "asset.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{

	Asset::Asset(const URL& url) : m_url(url)
	{
		m_asset_type = EAssetType::Invalid;
		m_archive_type = EArchiveType::Json;
		m_name = g_runtime_context.fileSystem()->basename(m_url);
	}

}