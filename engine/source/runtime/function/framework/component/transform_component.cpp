#include "transform_component.h"
#include "runtime/function/framework/entity/entity.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::TransformComponent>("TransformComponent")
	 .property("m_position", &Bamboo::TransformComponent::m_position)
	 .property("m_rotation", &Bamboo::TransformComponent::m_rotation)
	 .property("m_scale", &Bamboo::TransformComponent::m_scale);
}

CEREAL_REGISTER_TYPE(Bamboo::TransformComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::TransformComponent)

namespace Bamboo
{
	
	void TransformComponent::setPosition(const glm::vec3& position)
	{
		m_position = position;
	}

	void TransformComponent::setRotation(const glm::vec3& rotation)
	{
		m_rotation = rotation;
	}

	void TransformComponent::setScale(const glm::vec3& scale)
	{
		m_scale = scale;
	}

	bool TransformComponent::update(bool is_chain_dirty, const glm::mat4& parent_global_matrix)
	{
		// check is dirty
		if (m_last_transform != *(Transform*)this)
		{
			m_last_transform = *(Transform*)this;
			m_is_dirty = true;
		}

		// update local matrix
		if (m_is_dirty)
		{
			m_local_matrix = matrix();
		}

		// update global matrix
		if (is_chain_dirty)
		{
			m_global_matrix = parent_global_matrix * m_local_matrix;
		}

		is_chain_dirty |= m_is_dirty;
		m_is_dirty = false;
		return is_chain_dirty;
	}

	const glm::mat4& TransformComponent::getGlobalMatrix()
	{
		return m_parent.lock()->isRoot() ? m_local_matrix : m_global_matrix;
	}

}