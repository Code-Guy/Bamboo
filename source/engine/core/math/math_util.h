#pragma once

#include <glm/glm.hpp>

namespace Bamboo
{
	class MathUtil
	{
	public:
		static float pi();
		static float clamp(float val, float min, float max);
		static float mapRangeValueUnclamped(float val, float from_min, float from_max, float to_min, float to_max);
		static float mapRangeValueClamped(float val, float from_min, float from_max, float to_min, float to_max);

		static bool randomBool();
		static int randomInteger(int min, int max);
		static float randomFloat();
		static float randomFloat(float min, float max);
		static glm::vec3 randomUnitVector();
		static glm::vec3 randomRotation();
		static glm::vec3 randomPointInBoundingBox(const glm::vec3& center, const glm::vec3& extent);
	};
}