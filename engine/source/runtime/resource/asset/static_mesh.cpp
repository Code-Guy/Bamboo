#include "static_mesh.h"

namespace Bamboo
{

	StaticMesh::StaticMesh(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::StaticMesh;
		m_archive_type = EArchiveType::Json;
	}

	void StaticMesh::inflate()
	{

	}

}