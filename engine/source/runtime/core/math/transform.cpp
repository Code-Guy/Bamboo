#include "transform.h"

namespace Bamboo
{

	glm::mat4 Transform::matrix() const
	{
		glm::mat4 model_matrix(1.0f);

		model_matrix = glm::translate(model_matrix, m_position);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.x), k_forward_vector);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.y), k_right_vector);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.z), k_up_vector);
		model_matrix = glm::scale(model_matrix, m_scale);

		return model_matrix;
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