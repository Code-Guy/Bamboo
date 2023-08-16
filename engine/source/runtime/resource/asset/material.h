#pragma once

#include "runtime/resource/asset/texture_2d.h"

namespace Bamboo
{
	class Material : public Asset, public IAssetRef
	{
	public:
		std::shared_ptr<Texture2D> m_base_color_texure;
		std::shared_ptr<Texture2D> m_metallic_roughness_texure;
		std::shared_ptr<Texture2D> m_normal_texure;
		std::shared_ptr<Texture2D> m_occlusion_texure;
		std::shared_ptr<Texture2D> m_emissive_texure;

		glm::vec4 m_base_color_factor = glm::vec4(0.5f);
		glm::vec4 m_emissive_factor = glm::vec4(1.0f);
		float m_metallic_factor = 1.0f;
		float m_roughness_factor = 1.0f;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("asset_ref", cereal::base_class<IAssetRef>(this)));
			ar(cereal::make_nvp("base_color_factor", m_base_color_factor));
			ar(cereal::make_nvp("emissive_factor", m_emissive_factor));
			ar(cereal::make_nvp("metallic_factor", m_metallic_factor));
			ar(cereal::make_nvp("roughness_factor", m_roughness_factor));
		}

		virtual void bindRefs() override;
	};
}