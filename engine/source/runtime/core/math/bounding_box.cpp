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
}