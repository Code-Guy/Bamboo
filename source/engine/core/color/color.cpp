#include "color.h"

namespace Bamboo
{

	Color3 Color3::operator*(float v) const
	{
		return Color3(r * v, g * v, b * v);
	}

	void Color3::operator*=(float v)
	{
		r *= v; g *= v; b *= v;
	}

	glm::vec3 Color3::toVec3() const
	{
		return glm::vec3(r, g, b);
	}

	const Color3 Color3::White(1.0f, 1.0f, 1.0f);
	const Color3 Color3::Black(0.0f, 0.0f, 0.0f);
	const Color3 Color3::Red(1.0f, 0.0f, 0.0f);
	const Color3 Color3::Green(0.0f, 1.0f, 0.0f);
	const Color3 Color3::Blue(0.0f, 0.0f, 1.0f);
	const Color3 Color3::Yellow(1.0f, 1.0f, 0.0f);
	const Color3 Color3::Cyan(0.0f, 1.0f, 1.0f);
	const Color3 Color3::Magenta(1.0f, 0.0f, 1.0f);
	const Color3 Color3::Orange(0.95f, 0.61f, 0.07f);
	const Color3 Color3::Purple(0.66f, 0.03f, 0.89f);
	const Color3 Color3::Silver(0.74f, 0.76f, 0.78f);

	Color4 Color4::operator*(float v) const
	{
		return Color4(r * v, g * v, b * v, a * v);
	}

	void Color4::operator*=(float v)
	{
		r *= v; g *= v; b *= v; a*= v;
	}

	glm::vec4 Color4::toVec4()
	{
		return glm::vec4(r, g, b, a);
	}

	const Color4 Color4::White(1.0f, 1.0f, 1.0f);
	const Color4 Color4::Black(0.0f, 0.0f, 0.0f);
	const Color4 Color4::Transparent(0.0f, 0.0f, 0.0f, 0.0f);
	const Color4 Color4::Red(1.0f, 0.0f, 0.0f);
	const Color4 Color4::Green(0.0f, 1.0f, 0.0f);
	const Color4 Color4::Blue(0.0f, 0.0f, 1.0f);
	const Color4 Color4::Yellow(1.0f, 1.0f, 0.0f);
	const Color4 Color4::Cyan(0.0f, 1.0f, 1.0f);
	const Color4 Color4::Magenta(1.0f, 0.0f, 1.0f);
	const Color4 Color4::Orange(0.95f, 0.61f, 0.07f);
	const Color4 Color4::Purple(0.66f, 0.03f, 0.89f);
	const Color4 Color4::Silver(0.74f, 0.76f, 0.78f);
}