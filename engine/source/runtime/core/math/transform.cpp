#include "transform.h"

namespace Bamboo
{

	glm::mat4 Transform::matrix() const
	{
		glm::mat4 model_matrix(1.0f);

		model_matrix = glm::translate(model_matrix, m_position);
		model_matrix = rotationMatrix(model_matrix);
		model_matrix = glm::scale(model_matrix, m_scale);

		return model_matrix;
	}

	glm::mat4 Transform::rotationMatrix(const glm::mat4& base_matrix) const
	{
		glm::mat4 rotation_matrix = base_matrix;
		rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_rotation.x), k_forward_vector);
		rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_rotation.z), k_right_vector);
		rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_rotation.y), k_up_vector);

		return rotation_matrix;
	}

	glm::vec3 Transform::transformPosition(const glm::vec3& position)
	{
		return matrix() * glm::vec4(position, 1.0f);
	}

	glm::vec3 Transform::transformVector(const glm::vec3& vector)
	{
		return matrix() * glm::vec4(vector, 0.0f);
	}

	bool Transform::operator==(const Transform& other) const
	{
		return m_position == other.m_position &&
			m_rotation == other.m_rotation &&
			m_scale == other.m_scale;
	}

	bool Transform::operator!=(const Transform& other) const
	{
		return m_position != other.m_position ||
			m_rotation != other.m_rotation ||
			m_scale != other.m_scale;
	}

	glm::mat4 QTranform::matrix() const

	{
		glm::mat4 model_matrix(1.0f);

		model_matrix = glm::translate(model_matrix, m_position);
		model_matrix *= glm::mat4_cast(m_rotation);
		model_matrix = glm::scale(model_matrix, m_scale);

		return model_matrix;
	}

	bool QTranform::isIdentity() const
	{
		return glm::all(glm::epsilonEqual(m_position, k_zero_vector, k_epsilon)) &&
			glm::all(glm::epsilonEqual(m_rotation, k_zero_quat, k_epsilon)) &&
			glm::all(glm::epsilonEqual(m_scale, k_one_vector, k_epsilon));
	}

}