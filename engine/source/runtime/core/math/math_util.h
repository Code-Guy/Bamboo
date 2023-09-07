#pragma once

namespace Bamboo
{
	class MathUtil
	{
	public:
		static float clamp(float val, float min, float max);
		static float mapRangeValueUnclamped(float val, float from_min, float from_max, float to_min, float to_max);
		static float mapRangeValueClamped(float val, float from_min, float from_max, float to_min, float to_max);
	};
}