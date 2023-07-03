#pragma once

#include "component.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Bamboo
{
	class CameraComponent : public Component
	{
	public:
		CameraComponent(glm::vec3 position,
			float yaw, float pitch,
			float speed, float sensitivity);

		void setFovy(float fovy);
		void setAspectRatio(float aspect_ratio);
		void setClipping(float zNear, float zFar);

		glm::vec3 getPosition();
		glm::mat4 getViewMatrix();
		glm::mat4 getPerspectiveMatrix();
		glm::mat4 getViewPerspectiveMatrix();

		virtual void tick(float delta_time) override;

	private:
		void onKey(int key, int scancode, int action, int mods);
		void onCursorPos(double xpos, double ypos);
		void onMouseButton(int button, int action, int mods);

		void updatePose();

		glm::vec3 m_position;
		float m_yaw;
		float m_pitch;

		float m_speed;
		float m_sensitivity;

		float m_fovy;
		float m_aspect_ratio;
		float m_zNear;
		float m_zFar;

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