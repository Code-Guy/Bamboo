#include "skeletal_mesh.h"

namespace Bamboo
{

	SkeletalMesh::SkeletalMesh(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::SkeletalMesh;
		m_archive_type = EArchiveType::Json;
	}

}