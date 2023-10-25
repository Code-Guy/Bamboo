#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>

#include "engine/resource/serialization/serialization.h"

namespace Bamboo
{
	const glm::vec3 k_forward_vector = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 k_right_vector = glm::vec3(0.0f, 0.0f, 1.0f);
	const glm::vec3 k_up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 k_zero_vector = glm::vec3(0.0f);
	const glm::vec3 k_one_vector = glm::vec3(1.0f);
	const glm::quat k_zero_quat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	const float k_epsilon = 0.001f;

	struct Transform
	{
		glm::vec3 m_position = glm::vec3(0.0f);
		glm::vec3 m_rotation = glm::vec3(0.0f);
		glm::vec3 m_scale = glm::vec3(1.0f);

		glm::mat4 matrix() const;

		glm::vec3 transformPosition(const glm::vec3& position);
		glm::vec3 transformVector(const glm::vec3& vector);

		static glm::vec3 eulerAngles(const glm::quat& quat);
		static glm::quat quaterion(const glm::vec3& euler_angles);

		bool operator==(const Transform& other) const;
		bool operator!=(const Transform& other) const;
	};

	struct QTransform
	{
		glm::vec3 m_position = glm::vec3(0.0f);
		glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 m_scale = glm::vec3(1.0f);

		glm::mat4 matrix() const;
		void fromMatrix(const glm::mat4& m);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("position", m_position));
			ar(cereal::make_nvp("rotation", m_rotation));
			ar(cereal::make_nvp("scale", m_scale));
		}
	};
}