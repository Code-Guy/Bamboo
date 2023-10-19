#pragma once

#include "component.h"

namespace Bamboo
{
	enum class EMotionType
	{
		Static, Kinematic, Dynamic
	};

	class RigidbodyComponent : public Component
	{
	public:
		EMotionType m_motion_type;
		float m_friction = 0.2f;
		float m_restitution = 0.0f;
		float m_linear_damping = 0.05f;
		float m_angular_damping = 0.05f;
		float m_gravity_factor = 1.0f;

	protected:
		virtual void beginPlay() override;
		virtual void endPlay() override;

	private:
		REGISTER_REFLECTION(Component)

		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));
			ar(cereal::make_nvp("motion_type", m_motion_type));
			ar(cereal::make_nvp("friction", m_friction));
			ar(cereal::make_nvp("restitution", m_restitution));
			ar(cereal::make_nvp("linear_damping", m_linear_damping));
			ar(cereal::make_nvp("angular_damping", m_angular_damping));
			ar(cereal::make_nvp("gravity_factor", m_gravity_factor));
		}

		uint32_t m_body_id = UINT_MAX;
	};
}