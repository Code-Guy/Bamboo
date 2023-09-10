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
	 .property("fovy", &Bamboo::CameraComponent::m_fovy)
	 .property("near", &Bamboo::CameraComponent::m_near)
	 .property("far", &Bamboo::CameraComponent::m_far)
	 .property("move_speed", &Bamboo::CameraComponent::m_move_speed)
	 .property("turn_speed", &Bamboo::CameraComponent::m_turn_speed)
	 .property("zoom_speed", &Bamboo::CameraComponent::m_zoom_speed);
}

CEREAL_REGISTER_TYPE(Bamboo::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CameraComponent)

namespace Bamboo
{

	glm::vec3 CameraComponent::getPosition()
	{
		return m_transform_component->m_position;
	}

	glm::mat4 CameraComponent::getViewMatrix()
	{
		if (m_last_rotation != m_transform_component->m_rotation)
		{
			m_last_rotation = m_transform_component->m_rotation;
			updatePose();
		}

		return glm::lookAtRH(m_transform_component->m_position, m_transform_component->m_position + m_forward, m_up);
	}

	glm::mat4 CameraComponent::getPerspectiveMatrix()
	{
		glm::mat4 projMat = glm::perspectiveRH_ZO(glm::radians(m_fovy), m_aspect_ratio, m_near, m_far);
		projMat[1][1] *= -1.0f;

		return projMat;
	}

	glm::mat4 CameraComponent::getViewPerspectiveMatrix()
	{
		return getPerspectiveMatrix() * getViewMatrix();
	}

	glm::mat4 CameraComponent::getViewMatrixNoTranslation()
	{
		return glm::mat4(glm::mat3(getViewMatrix()));
	}

	glm::mat4 CameraComponent::getPerspectiveMatrixNoInverted()
	{
		return glm::perspectiveRH_ZO(glm::radians(m_fovy), m_aspect_ratio, m_near, m_far);
	}

	void CameraComponent::tick(float delta_time)
	{
		float offset = m_move_speed * delta_time;

		if (m_mouse_right_button_pressed)
		{
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
	}

	void CameraComponent::inflate()
	{
		// get transform component
		m_transform_component = m_parent.lock()->getComponent(TransformComponent);

		// bind camera input callbacks
		g_runtime_context.eventSystem()->addListener(EEventType::WindowKey, std::bind(&CameraComponent::onKey, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::WindowCursorPos, std::bind(&CameraComponent::onCursorPos, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::WindowMouseButton, std::bind(&CameraComponent::onMouseButton, this, std::placeholders::_1));
		g_runtime_context.eventSystem()->addListener(EEventType::WindowScroll, std::bind(&CameraComponent::onScroll, this, std::placeholders::_1));

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
		if (!m_mouse_right_button_pressed)
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
		yaw += xoffset;
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
		else if (mouse_button_event->action == GLFW_RELEASE && mouse_button_event->button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_mouse_right_button_pressed = false;
			m_last_xpos = m_last_ypos = 0.0;
		}
	}

	void CameraComponent::onScroll(const std::shared_ptr<class Event>& event)
	{
		const WindowScrollEvent* scroll_event = static_cast<const WindowScrollEvent*>(event.get());
		if (m_enabled)
		{
			m_transform_component->m_position += m_forward * (float)scroll_event->yoffset * m_zoom_speed;
		}
	}

	void CameraComponent::updatePose()
	{
		m_forward = m_transform_component->getForwardVector();
		m_right = glm::cross(m_forward, k_up_vector);
		m_up = glm::cross(m_right, m_forward);
	}

}