#include "bounding_box.h"

namespace Bamboo
{
	BoundingBox BoundingBox::transform(const glm::mat4& m)
	{
		glm::vec3 tmin = glm::vec3(m[3]);
		glm::vec3 tmax = tmin;
		glm::vec3 v0, v1;

		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * min.x;
		v1 = right * max.x;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * min.y;
		v1 = up * max.y;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * min.z;
		v1 = back * max.z;
		tmin += glm::min(v0, v1);
		tmax += glm::max(v0, v1);

		return BoundingBox{tmin, tmax};
	}
}