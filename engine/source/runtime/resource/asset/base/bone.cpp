#include "bone.h"


namespace Bamboo
{

	void Bone::setRotation(const glm::quat& quat)
	{
		m_anim_transform.m_rotation = quat;
	}

	void Bone::setTranslation(const glm::vec3& translation)
	{
		m_anim_transform.m_position = translation;
	}

	void Bone::setScale(const glm::vec3& scale)
	{
		m_anim_transform.m_scale = scale;
	}

	void Bone::update(const glm::mat4& parent_global_bind_pose_matrix)
	{
		glm::mat4 anim_matrix = m_local_bind_pose_matrix;
		if (!m_anim_transform.isIdentity())
		{
			anim_matrix = m_anim_transform.matrix();
		}

		m_global_bind_pose_matrix = parent_global_bind_pose_matrix * anim_matrix;
	}

	glm::mat4 Bone::matrix() const
	{
		return m_global_bind_pose_matrix * m_global_inverse_bind_pose_matrix;
	}

}