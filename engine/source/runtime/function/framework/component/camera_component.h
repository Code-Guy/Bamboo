#pragma once

#include "component.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace Bamboo
{
	class CameraComponent : public Component
	{
	public:
		CameraComponent();

		glm::mat4 getViewMatrix();
		glm::mat4 getPerspectiveMatrix();
		glm::mat4 getViewPerspectiveMatrix();

		virtual void tick(float delta_time) override;
		virtual void inflate() override;

		// pose
		glm::vec3 m_position;
		float m_yaw;
		float m_pitch;

		// projection
		float m_fovy;
		float m_aspect_ratio;
		float m_near;
		float m_far;

		// movement
		float m_speed;
		float m_sensitivity;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			// pose
			ar(cereal::make_nvp("position", m_position));
			ar(cereal::make_nvp("yaw", m_yaw));
			ar(cereal::make_nvp("pitch", m_pitch));

			// projection
			ar(cereal::make_nvp("fovy", m_fovy));
			ar(cereal::make_nvp("aspect_ratio", m_aspect_ratio));
			ar(cereal::make_nvp("near", m_near));
			ar(cereal::make_nvp("far", m_far));

			// movement
			ar(cereal::make_nvp("speed", m_speed));
			ar(cereal::make_nvp("sensitivity", m_sensitivity));
		}

		void onKey(int key, int scancode, int action, int mods);
		void onCursorPos(double xpos, double ypos);
		void onMouseButton(int button, int action, int mods);

		void updatePose();

		glm::vec3 m_forward;
		glm::vec3 m_right;
		glm::vec3 m_up;

		bool m_move_forward;
		bool m_move_back;
		bool m_move_left;
		bool m_move_right;

		bool m_mouse_right_button_pressed;
	};
}

CEREAL_REGISTER_TYPE(Bamboo::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CameraComponent)