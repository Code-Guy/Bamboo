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

		glm::vec4 m_base_color_factor = glm::vec4(1.0f);
		glm::vec4 m_emissive_factor = glm::vec4(1.0f);
		float m_metallic_factor = 1.0f;
		float m_roughness_factor = 1.0f;

	protected:
		virtual void bindRefs() override;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::base_class<IAssetRef>(this));
			ar(m_base_color_factor, m_emissive_factor, m_metallic_factor, m_roughness_factor);
		}
	};
}