#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Bamboo
{
	struct BoundingBox
	{
		glm::vec3 min;
		glm::vec3 max;

		BoundingBox transform(const glm::mat4& m);
	};
}