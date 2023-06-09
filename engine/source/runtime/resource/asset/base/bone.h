#pragma once

#include "runtime/core/math/transform.h"
#include "runtime/resource/asset/base/asset.h"

#define INVALID_BONE_INDEX 255

namespace Bamboo
{
	class Bone
	{
	public:
		std::string m_name;
		uint8_t m_parent = INVALID_BONE_INDEX;
		std::vector<uint8_t> m_children;

		glm::mat4 m_local_bind_pose_matrix;
		glm::mat4 m_global_inverse_bind_pose_matrix;
		glm::mat4 m_global_bind_pose_matrix;

		void setRotation(const glm::quat& quat);
		void setTranslation(const glm::vec3& translation);
		void setScale(const glm::vec3& scale);

		// update bone matrix
		void update(const glm::mat4& parent_global_bind_pose_matrix);

		// get bone to world matrix of this bone
		glm::mat4 matrix() const;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(m_name, m_parent, m_children);
			ar(m_local_bind_pose_matrix, m_global_inverse_bind_pose_matrix);
		}

		QTranform m_anim_transform;
	};
}