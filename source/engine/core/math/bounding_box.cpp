#include "bounding_box.h"

namespace Bamboo
{
	BoundingBox BoundingBox::transform(const glm::mat4& m)
	{
		glm::vec3 tmin = glm::vec3(m[3]);
		glm::vec3 tmax = tmin;
		glm::vec3 v0, v1;

		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * m_min.x;
		v1 = right * m_max.x;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * m_min.y;
		v1 = up * m_max.y;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * m_min.z;
		v1 = back * m_max.z;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		return BoundingBox{tmin, tmax};
	}

	void BoundingBox::combine(const BoundingBox& other)
	{
		m_min.x = std::min(m_min.x, other.m_min.x);
		m_min.y = std::min(m_min.y, other.m_min.y);
		m_min.z = std::min(m_min.z, other.m_min.z);
		m_max.x = std::max(m_max.x, other.m_max.x);
		m_max.y = std::max(m_max.y, other.m_max.y);
		m_max.z = std::max(m_max.z, other.m_max.z);
	}

	void BoundingBox::combine(const glm::vec3& position)
	{
		m_min.x = std::min(m_min.x, position.x);
		m_min.y = std::min(m_min.y, position.y);
		m_min.z = std::min(m_min.z, position.z);
		m_max.x = std::max(m_max.x, position.x);
		m_max.y = std::max(m_max.y, position.y);
		m_max.z = std::max(m_max.z, position.z);
	}

	glm::vec3 BoundingBox::center() const
	{
		return (m_min + m_max) * 0.5f;
	}

	glm::vec3 BoundingBox::extent() const
	{
		return (m_max - m_min) * 0.5f;
	}

}