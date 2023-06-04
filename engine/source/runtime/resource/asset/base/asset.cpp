#include "asset.h"

namespace Bamboo
{

	Asset::Asset(const URL& url) : m_url(url)
	{
		m_asset_type = EAssetType::Invalid;
		m_archive_type = EArchiveType::Json;
	}

}