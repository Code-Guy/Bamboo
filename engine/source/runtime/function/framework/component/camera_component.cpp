#include "camera_component.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include <algorithm>
#include <functional>

namespace Bamboo
{
	CameraComponent::CameraComponent(glm::vec3 position, 
		float yaw, float pitch, 
		float speed, float sensitivity) :
		m_position(position),
		m_yaw(yaw), m_pitch(pitch),
		m_speed(speed), m_sensitivity(sensitivity)
	{
		m_move_forward = false;
		m_move_back = false;
		m_move_left = false;
		m_move_right = false;
		m_mouse_right_button_pressed = false;

		// bind camera input callbacks
		g_runtime_context.windowSystem()->registerOnKeyFunc(std::bind(&CameraComponent::onKey, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		g_runtime_context.windowSystem()->registerOnKeyFunc(std::bind(&CameraComponent::onCursorPos, this,
			std::placeholders::_1, std::placeholders::_2));
		g_runtime_context.windowSystem()->registerOnKeyFunc(std::bind(&CameraComponent::onMouseButton, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		updatePose();
	}

	void CameraComponent::setFovy(float fovy)
	{
		m_fovy = fovy;
	}

	void CameraComponent::setAspectRatio(float aspect_ratio)
	{
		m_aspect_ratio = aspect_ratio;
	}

	void CameraComponent::setClipping(float zNear, float zFar)
	{
		m_zNear = zNear;
		m_zFar = zFar;
	}

	void CameraComponent::tick(float delta_time)
	{
		float offset = m_speed * delta_time;

		if (m_move_forward)
		{
			m_position += m_forward * offset;
		}
		if (m_move_back)
		{
			m_position -= m_forward * offset;
		}
		if (m_move_left)
		{
			m_position -= m_right * offset;
		}
		if (m_move_right)
		{
			m_position += m_right * offset;
		}
	}

	glm::vec3 CameraComponent::getPosition()
	{
		return m_position;
	}

	glm::mat4 CameraComponent::getViewMatrix()
	{
		return glm::lookAt(m_position, m_position + m_forward, m_up);
	}

	glm::mat4 CameraComponent::getPerspectiveMatrix()
	{
		glm::mat4 projMat = glm::perspective(glm::radians(m_fovy), m_aspect_ratio, m_zNear, m_zFar);
		projMat[1][1] *= -1.0f;

		return projMat;
	}

	glm::mat4 CameraComponent::getViewPerspectiveMatrix()
	{
		return getPerspectiveMatrix() * getViewMatrix();
	}

	void CameraComponent::onKey(int key, int scancode, int action, int mods)
	{
		if (action != GLFW_PRESS && action != GLFW_RELEASE)
		{
			return;
		}

		bool is_pressed = action == GLFW_PRESS;
		if (key == GLFW_KEY_W)
		{
			m_move_forward = is_pressed;
		}
		if (key == GLFW_KEY_S)
		{
			m_move_back = is_pressed;
		}
		if (key == GLFW_KEY_A)
		{
			m_move_left = is_pressed;
		}
		if (key == GLFW_KEY_D)
		{
			m_move_right = is_pressed;
		}
	}

	void CameraComponent::onCursorPos(double xpos, double ypos)
	{
		if (!m_mouse_right_button_pressed)
		{
			return;
		}

		xpos *= m_sensitivity;
		ypos *= m_sensitivity;

		m_yaw -= ypos;
		m_pitch -= ypos;
		m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

		updatePose();
	}

	void CameraComponent::onMouseButton(int button, int action, int mods)
	{
		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_mouse_right_button_pressed = true;
		}
		if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_mouse_right_button_pressed = false;
		}
	}

	void CameraComponent::updatePose()
	{
		m_forward.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
		m_forward.y = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
		m_forward.z = std::sin(glm::radians(m_pitch));
		m_forward = glm::normalize(m_forward);

		m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 0.0f, 1.0f)));
		m_up = glm::normalize(glm::cross(m_right, m_forward));
	}
}