#pragma once

#include "runtime/resource/asset/texture_2d.h"

namespace Bamboo
{
	class Material : public Asset
	{
	public:
		Material(const URL& url);

		std::shared_ptr<Texture2D> m_base_color_texure;
		std::shared_ptr<Texture2D> m_metallic_roughness_texure;
		std::shared_ptr<Texture2D> m_normal_texure;
		std::shared_ptr<Texture2D> m_occlusion_texure;
		std::shared_ptr<Texture2D> m_emissive_texure;

		glm::vec4 m_base_color_factor;
		glm::vec4 m_emissive_factor;
		float m_metallic_factor;
		float m_roughness_factor;

	private:
		
	};
}