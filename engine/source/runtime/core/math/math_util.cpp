#include "math_util.h"
#include <cmath>
#include <algorithm>

namespace Bamboo
{
	
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

}