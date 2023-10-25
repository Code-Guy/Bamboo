#include "math_util.h"
#include <cmath>
#include <algorithm>
#include <effolkronium/random.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using Random = effolkronium::random_static;

namespace Bamboo
{
	
	float MathUtil::pi()
	{
		return 3.1415926f;
	}

	float MathUtil::clamp(float val, float min, float max)
	{
		return std::min(std::max(val, min), max);
	}

	float MathUtil::mapRangeValueUnclamped(float val, float from_min, float from_max, float to_min, float to_max)
	{
		return (val - from_min) / (from_max - from_min) * (to_max - to_min) + to_min;
	}

	float MathUtil::mapRangeValueClamped(float val, float from_min, float from_max, float to_min, float to_max)
	{
		val = clamp(val, from_min, from_max);
		return mapRangeValueUnclamped(val, from_min, from_max, to_min, to_max);
	}

	bool MathUtil::randomBool()
	{
		return Random::get<bool>();
	}

	int MathUtil::randomInteger(int min, int max)
	{
		return Random::get(min, max);
	}

	float MathUtil::randomFloat()
	{
		return Random::get(0.0f, 1.0f);
	}

	float MathUtil::randomFloat(float min, float max)
	{
		return Random::get(min, max);
	}

	glm::vec3 MathUtil::randomUnitVector()
	{
		return glm::normalize(glm::vec3(
			randomFloat(-1.0f, 1.0f),
			randomFloat(-1.0f, 1.0f),
			randomFloat(-1.0f, 1.0f)
		));
	}

	glm::vec3 MathUtil::randomRotation()
	{
		return glm::vec3(
			randomFloat(-180.0f, 180.0f),
			randomFloat(-180.0f, 180.0f),
			randomFloat(-180.0f, 180.0f)
		);
	}

	glm::vec3 MathUtil::randomPointInBoundingBox(const glm::vec3& center, const glm::vec3& extent)
	{
		return glm::vec3(
			randomFloat(center.x - extent.x, center.x + extent.x),
			randomFloat(center.y - extent.y, center.y + extent.y),
			randomFloat(center.z - extent.z, center.z + extent.z)
		);
	}

}