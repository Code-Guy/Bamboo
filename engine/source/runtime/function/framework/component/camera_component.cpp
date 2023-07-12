#include "camera_component.h"
#include "transform_component.h"
#include "runtime/core/base/macro.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/entity/entity.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <functional>

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::CameraComponent>("CameraComponent")
	 .property("m_fovy", &Bamboo::CameraComponent::m_fovy)
	 .property("m_near", &Bamboo::CameraComponent::m_near)
	 .property("m_far", &Bamboo::CameraComponent::m_far)
	 .property("m_move_speed", &Bamboo::CameraComponent::m_move_speed)
	 .property("m_turn_speed", &Bamboo::CameraComponent::m_turn_speed)
	 .property("m_zoom_speed", &Bamboo::CameraComponent::m_zoom_speed);
}

CEREAL_REGISTER_TYPE(Bamboo::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CameraComponent)

namespace Bamboo
{

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
		m_transform_component = m_parent.lock()->getComponent(TransformComponent);

		// bind camera input callbacks
		g_runtime_context.eventSystem()->addListener(EventType::WindowKey, std::bind(&CameraComponent::onKey, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::WindowCursorPos, std::bind(&CameraComponent::onCursorPos, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::WindowMouseButton, std::bind(&CameraComponent::onMouseButton, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EventType::WindowScroll, std::bind(&CameraComponent::onScroll, this, std::placeholders::_1));

		// update camera pose
		updatePose();
	}

	void CameraComponent::onKey(const std::shared_ptr<class Event>& event)
	{
		const WindowKeyEvent* key_event = static_cast<const WindowKeyEvent*>(event.get());
		if (key_event->action != GLFW_PRESS && key_event->action != GLFW_RELEASE)
		{
			return;
		}

		bool is_pressed = key_event->action == GLFW_PRESS;
		if (key_event->key == GLFW_KEY_W)
		{
			m_move_forward = is_pressed;
		}
		if (key_event->key == GLFW_KEY_S)
		{
			m_move_back = is_pressed;
		}
		if (key_event->key == GLFW_KEY_A)
		{
			m_move_left = is_pressed;
		}
		if (key_event->key == GLFW_KEY_D)
		{
			m_move_right = is_pressed;
		}
		if (key_event->key == GLFW_KEY_Q)
		{
			m_move_up = is_pressed;
		}
		if (key_event->key == GLFW_KEY_E)
		{
			m_move_down = is_pressed;
		}
	}

	void CameraComponent::onCursorPos(const std::shared_ptr<class Event>& event)
	{
		const WindowCursorPosEvent* cursor_pos_event = static_cast<const WindowCursorPosEvent*>(event.get());
		m_mouse_in_content = isInContentRegion(cursor_pos_event->xpos, cursor_pos_event->ypos);
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
			xoffset = cursor_pos_event->xpos - m_last_xpos;
		}
		if (m_last_ypos != 0.0)
		{
			yoffset = cursor_pos_event->ypos - m_last_ypos;
		}
		m_last_xpos = cursor_pos_event->xpos;
		m_last_ypos = cursor_pos_event->ypos;

		xoffset *= m_turn_speed;
		yoffset *= m_turn_speed;

		float& yaw = m_transform_component->m_rotation.y;
		float& pitch = m_transform_component->m_rotation.z;
		yaw -= xoffset;
		pitch -= yoffset;
		pitch = std::clamp(pitch, -89.0f, 89.0f);

		updatePose();
	}

	void CameraComponent::onMouseButton(const std::shared_ptr<class Event>& event)
	{
		const WindowMouseButtonEvent* mouse_button_event = static_cast<const WindowMouseButtonEvent*>(event.get());
		if (mouse_button_event->action == GLFW_PRESS && mouse_button_event->button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_mouse_right_button_pressed = true;
		}
		if (mouse_button_event->action == GLFW_RELEASE && mouse_button_event->button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_mouse_right_button_pressed = false;
			m_last_xpos = m_last_ypos = 0.0;
		}
	}

	void CameraComponent::onScroll(const std::shared_ptr<class Event>& event)
	{
		const WindowScrollEvent* scroll_event = static_cast<const WindowScrollEvent*>(event.get());
		if (!m_mouse_in_content)
		{
			return;
		}

		m_transform_component->m_position += m_forward * (float)scroll_event->yoffset * m_zoom_speed;
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