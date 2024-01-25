#pragma once

#include <glm/glm.hpp>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>

namespace Bamboo
{
	class Color3
	{
	public:
		union
		{
			struct
			{
				float r, g, b;
			};

			float c[3];
		};

		Color3(float r = 0.0f, float g = 0.0f, float b = 0.0f) : r(r), g(g), b(b) {}

		float& operator[](uint32_t index) { return c[index]; }
		float operator[](uint32_t index) const { return c[index]; }

		Color3 operator*(float v) const;
		void operator*=(float v);

		glm::vec3 toVec3() const;

		float* data() { return c; }

		static const Color3 White;
		static const Color3 Black;
		static const Color3 Red;
		static const Color3 Green;
		static const Color3 Blue;
		static const Color3 Yellow;
		static const Color3 Cyan;
		static const Color3 Magenta;
		static const Color3 Orange;
		static const Color3 Purple;
		static const Color3 Silver;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("r", r));
			ar(cereal::make_nvp("g", g));
			ar(cereal::make_nvp("b", b));
		}
	};

	class Color4
	{
	public:
		union
		{
			struct
			{
				float r, g, b, a;
			};

			float c[4];
		};

		Color4(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

		float& operator[](uint32_t index) { return c[index]; }
		float operator[](uint32_t index) const { return c[index]; }

		Color4 operator*(float v) const;
		void operator*=(float v);

		glm::vec4 toVec4();
		float* data() { return c; }

		static const Color4 White;
		static const Color4 Black;
		static const Color4 Transparent;
		static const Color4 Red;
		static const Color4 Green;
		static const Color4 Blue;
		static const Color4 Yellow;
		static const Color4 Cyan;
		static const Color4 Magenta;
		static const Color4 Orange;
		static const Color4 Purple;
		static const Color4 Silver;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("r", r));
			ar(cereal::make_nvp("g", g));
			ar(cereal::make_nvp("b", b));
			ar(cereal::make_nvp("a", a));
		}
	};
}