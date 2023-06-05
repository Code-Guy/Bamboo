#pragma once

#include "runtime/resource/asset/texture_2d.h"

namespace Bamboo
{
	class Material : public Asset
	{
	public:
		Material() = default;
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
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(cereal::base_class<Asset>(this));
			ar(m_base_color_factor, m_emissive_factor, m_metallic_factor, m_roughness_factor);
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);
		}
	};

	RTTR_REGISTRATION
	{
	rttr::registration::class_<Material>("Material")
		 .constructor<>()
		 .property("m_base_color_texure", &Material::m_base_color_texure)
		 .property("m_metallic_roughness_texure", &Material::m_metallic_roughness_texure)
		 .property("m_normal_texure", &Material::m_normal_texure)
		 .property("m_occlusion_texure", &Material::m_occlusion_texure)
		 .property("m_emissive_texure", &Material::m_emissive_texure);
	}
}