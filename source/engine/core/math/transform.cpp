#include "transform.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <Jolt/Jolt.h>

namespace Bamboo
{

	glm::mat4 Transform::matrix() const
	{
		glm::mat4 model_matrix(1.0f);

		model_matrix = glm::translate(model_matrix, m_position);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.z), k_right_vector);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.y), k_up_vector);
		model_matrix = glm::rotate(model_matrix, glm::radians(m_rotation.x), k_forward_vector);
		model_matrix = glm::scale(model_matrix, m_scale);

		return model_matrix;
	}

	glm::vec3 Transform::transformPosition(const glm::vec3& position)
	{
		return matrix() * glm::vec4(position, 1.0f);
	}

	glm::vec3 Transform::transformVector(const glm::vec3& vector)
	{
		return matrix() * glm::vec4(vector, 0.0f);
	}

	glm::vec3 Transform::eulerAngles(const glm::quat& quat)
	{
		JPH::Vec3 euler_angles = JPH::Quat(quat.x, quat.y, quat.z, quat.w).GetEulerAngles();
		return glm::degrees(glm::vec3(euler_angles[0], euler_angles[1], euler_angles[2]));
	}

	glm::quat Transform::quaterion(const glm::vec3& euler_angles)
	{
		JPH::Quat quat = JPH::Quat::sEulerAngles(JPH::Vec3(
			glm::radians(euler_angles.x),
			glm::radians(euler_angles.y),
			glm::radians(euler_angles.z)
		));
		return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
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

	glm::mat4 QTransform::matrix() const

	{
		glm::mat4 model_matrix(1.0f);

		model_matrix = glm::translate(model_matrix, m_position);
		model_matrix *= glm::mat4_cast(m_rotation);
		model_matrix = glm::scale(model_matrix, m_scale);

		return model_matrix;
	}

	void QTransform::fromMatrix(const glm::mat4& m)
	{	
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(m, m_scale, m_rotation, m_position, skew, perspective);
	}

}