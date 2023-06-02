#pragma once

#include "runtime/resource/asset/texture_2d.h"
#include <memory>

namespace Bamboo
{
	class Material
	{
		std::shared_ptr<Texture2D> m_base_color_tex;
		std::shared_ptr<Texture2D> m_metallic_roughness_color_tex;
		std::shared_ptr<Texture2D> m_normal_tex;

		float m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;
	};
}