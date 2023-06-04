#include "material.h"

namespace Bamboo
{

	Material::Material(const URL& url) : Asset(url)
	{
		m_asset_type = EAssetType::Material;
		m_archive_type = EArchiveType::Json;

		m_base_color_factor = glm::vec4(1.0f);
		m_emissive_factor = glm::vec4(1.0f);
		m_metallic_factor = 1.0f;
		m_roughness_factor = 1.0f;
	}

}

