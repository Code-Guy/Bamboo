#include "bone.h"


namespace Bamboo
{

	void Bone::setRotation(const glm::quat& quat)
	{
		m_local_bind_pose_transform.m_rotation = quat;
	}

	void Bone::setTranslation(const glm::vec3& translation)
	{
		m_local_bind_pose_transform.m_position = translation;
	}

	void Bone::setScale(const glm::vec3& scale)
	{
		m_local_bind_pose_transform.m_scale = scale;
	}

	void Bone::update(const glm::mat4& parent_global_bind_pose_matrix)
	{
		m_global_bind_pose_matrix = parent_global_bind_pose_matrix * m_local_bind_pose_transform.matrix();
	}

	glm::mat4 Bone::matrix() const
	{
		return m_global_bind_pose_matrix * m_global_inverse_bind_pose_matrix;
	}

}