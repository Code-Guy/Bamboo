#pragma once

#include "component.h"
#include "engine/core/math/transform.h"
#include "engine/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	class TransformComponent : public Component, public Transform
	{
	public:
		~TransformComponent();

		void setPosition(const glm::vec3& position);
		void setRotation(const glm::vec3& rotation);
		void setScale(const glm::vec3& scale);

		bool update(bool is_chain_dirty = false, const glm::mat4& parent_global_matrix = glm::mat4(1.0));
		VmaBuffer updateUniformBuffer(const glm::mat4& vp);

		const glm::mat4& getGlobalMatrix();
		glm::vec3 getForwardVector();

	private:
		REGISTER_REFLECTION(Component)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("position", m_position));
			ar(cereal::make_nvp("rotation", m_rotation));
			ar(cereal::make_nvp("scale", m_scale));
		}

		bool m_is_dirty = true;
		glm::mat4 m_local_matrix;
		glm::mat4 m_global_matrix;

		Transform m_last_transform;
		std::vector<VmaBuffer> m_transform_ubs;
	};
}