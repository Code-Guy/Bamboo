#pragma once

#include <glm/glm.hpp>
#include <cereal/access.hpp>
#include <limits>

namespace Bamboo
{
	struct BoundingBox
	{
		glm::vec3 m_min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 m_max = glm::vec3(std::numeric_limits<float>::min());

		BoundingBox transform(const glm::mat4& m);

	private:
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(m_min, m_max);
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);
		}
	};
}