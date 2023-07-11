#include "camera_component.h"
#include "transform_component.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/framework/entity/entity.h"

#include <algorithm>
#include <functional>

CEREAL_REGISTER_TYPE(Bamboo::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CameraComponent)

namespace Bamboo
{
	CameraComponent::CameraComponent()
	{
		m_move_forward = false;
		m_move_back = false;
		m_move_left = false;
		m_move_right = false;
		m_move_up = false;
		m_move_down = false;

		m_mouse_in_content = false;
		m_mouse_right_button_pressed = false;

		m_last_xpos = m_last_ypos = 0.0f;
	}

	void CameraComponent::setContentRegion(const glm::uvec4& content_region)
	{
		m_content_region = content_region;
		m_aspect_ratio = (float)m_content_region.z / m_content_region.w;
	}

	glm::mat4 CameraComponent::getViewMatrix()
	{
		return glm::lookAt(m_transform_component->m_position, m_transform_component->m_position + m_forward, m_up);
	}

	glm::mat4 CameraComponent::getPerspectiveMatrix()
	{
		glm::mat4 projMat = glm::perspective(glm::radians(m_fovy), m_aspect_ratio, m_near, m_far);
		projMat[1][1] *= -1.0f;

		return projMat;
	}

	glm::mat4 CameraComponent::getViewPerspectiveMatrix()
	{
		return getPerspectiveMatrix() * getViewMatrix();
	}

	void CameraComponent::tick(float delta_time)
	{
		float offset = m_move_speed * delta_time;

		if (m_move_forward)
		{
			m_transform_component->m_position += m_forward * offset;
		}
		if (m_move_back)
		{
			m_transform_component->m_position -= m_forward * offset;
		}
		if (m_move_left)
		{
			m_transform_component->m_position -= m_right * offset;
		}
		if (m_move_right)
		{
			m_transform_component->m_position += m_right * offset;
		}
		if (m_move_up)
		{
			m_transform_component->m_position += k_up_vector * offset;
		}
		if (m_move_down)
		{
			m_transform_component->m_position -= k_up_vector * offset;
		}
	}

	void CameraComponent::inflate()
	{
		// get transform component
		m_transform_component = m_parent.lock()->getComponent<TransformComponent>();

		// bind camera input callbacks
		g_runtime_context.windowSystem()->registerOnKeyFunc(std::bind(&CameraComponent::onKey, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		g_runtime_context.windowSystem()->registerOnCursorPosFunc(std::bind(&CameraComponent::onCursorPos, this,
			std::placeholders::_1, std::placeholders::_2));
		g_runtime_context.windowSystem()->registerOnMouseButtonFunc(std::bind(&CameraComponent::onMouseButton, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		g_runtime_context.windowSystem()->registerOnScrollFunc(std::bind(&CameraComponent::onScroll, this,
			std::placeholders::_1, std::placeholders::_2));

		// update camera pose
		updatePose();
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
		if (key == GLFW_KEY_Q)
		{
			m_move_up = is_pressed;
		}
		if (key == GLFW_KEY_E)
		{
			m_move_down = is_pressed;
		}
	}

	void CameraComponent::onCursorPos(double xpos, double ypos)
	{
		m_mouse_in_content = isInContentRegion(xpos, ypos);
		if (!m_mouse_right_button_pressed)
		{
			return;
		}

		if (!m_mouse_in_content)
		{
			m_last_xpos = m_last_ypos = 0.0f;
			return;
		}

		double xoffset = 0.0;
		double yoffset = 0.0f;

		if (m_last_xpos != 0.0)
		{
			xoffset = xpos - m_last_xpos;
		}
		if (m_last_ypos != 0.0)
		{
			yoffset = ypos - m_last_ypos;
		}
		m_last_xpos = xpos;
		m_last_ypos = ypos;

		xoffset *= m_turn_speed;
		yoffset *= m_turn_speed;

		float& yaw = m_transform_component->m_rotation.y;
		float& pitch = m_transform_component->m_rotation.z;
		yaw -= xoffset;
		pitch -= yoffset;
		pitch = std::clamp(pitch, -89.0f, 89.0f);

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
			m_last_xpos = m_last_ypos = 0.0;
		}
	}

	void CameraComponent::onScroll(double xoffset, double yoffset)
	{
		if (!m_mouse_in_content)
		{
			return;
		}

		m_transform_component->m_position += m_forward * (float)yoffset * m_zoom_speed;
	}

	void CameraComponent::updatePose()
	{
		float& yaw = m_transform_component->m_rotation.y;
		float& pitch = m_transform_component->m_rotation.z;
		m_forward.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
		m_forward.y = std::sin(glm::radians(pitch));
		m_forward.z = -std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
		m_forward = glm::normalize(m_forward);

		m_right = glm::normalize(glm::cross(m_forward, k_up_vector));
		m_up = glm::normalize(glm::cross(m_right, m_forward));
	}

	bool CameraComponent::isInContentRegion(double xpos, double ypos)
	{
		return xpos > m_content_region.x && xpos < m_content_region.x + m_content_region.z &&
			ypos > m_content_region.y && ypos < m_content_region.y + m_content_region.w;
	}

}