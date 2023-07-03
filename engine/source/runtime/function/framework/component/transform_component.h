#pragma once

#include "component.h"
#include "runtime/core/math/transform.h"

namespace Bamboo
{
	class TransformComponent : public Transform, public Component
	{
	public:
		glm::mat4 local_matrix = glm::mat4(1.0f);
		glm::mat4 world_matrix = glm::mat4(1.0f);
	};
}