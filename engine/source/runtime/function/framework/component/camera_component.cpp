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
	 .property("zoom_speed", &Bamboo::CameraComponent::m_zoom_speed)
	 .property("exposure", &Bamboo::CameraComponent::m_exposure);
}

CEREAL_REGISTER_TYPE(Bamboo::CameraComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::Component, Bamboo::CameraComponent)

namespace Bamboo
{

	CameraComponent::CameraComponent()
	{
		m_projection_type = EProjectionType::Perspective;
		m_ortho_width = 10.0f;
	}

	CameraComponent::~CameraComponent()
	{
		g_runtime_context.eventSystem()->removeListener(m_key_event_handle);
		g_runtime_context.eventSystem()->removeListener(m_cursor_pos_event_handle);
		g_runtime_context.eventSystem()->removeListener(m_scroll_event_handle);
	}

	glm::vec3 CameraComponent::getPosition()
	{
		return m_transform_component->m_position;
	}

	glm::mat4 CameraComponent::getViewMatrix()
	{
		return m_view_matrix;
	}

	glm::mat4 CameraComponent::getProjectionMatrix()
	{
		return m_projection_matrix;
	}

	glm::mat4 CameraComponent::getProjectionMatrix(EProjectionType projection_type)
	{
		glm::mat4 projection_matrix(1.0);
		if (projection_type == EProjectionType::Perspective)
		{
			projection_matrix = glm::perspectiveRH_ZO(glm::radians(m_fovy), m_aspect_ratio, m_near, m_far);
		}
		else if (projection_type == EProjectionType::Orthographic)
		{
			float ortho_height = m_ortho_width / m_aspect_ratio;
			projection_matrix = glm::orthoRH_ZO(-m_ortho_width, m_ortho_width, -ortho_height, ortho_height, -m_ortho_width, m_far);
		}

		return getProjectionMatrixYInverted(projection_matrix);
	}

	glm::mat4 CameraComponent::getViewProjectionMatrix()
	{
		return m_view_projection_matrix;
	}

	glm::mat4 CameraComponent::getViewMatrixNoTranslation()
	{
		return glm::mat4(glm::mat3(m_view_matrix));
	}

	glm::mat4 CameraComponent::getProjectionMatrixNoYInverted()
	{
		return getProjectionMatrixYInverted(m_projection_matrix);
	}

	void CameraComponent::setInput(bool mouse_right_button_pressed, bool mouse_focused)
	{
		m_mouse_right_button_pressed = mouse_right_button_pressed;
		m_mouse_focused = mouse_focused;
	}

	void CameraComponent::tick(float delta_time)
	{
		// update camera position
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

		// update camera view/projection matrix
		m_view_matrix = glm::lookAtRH(m_transform_component->m_position, m_transform_component->m_position + m_forward, m_up);
		m_projection_matrix = getProjectionMatrix(m_projection_type);
		m_view_projection_matrix = m_projection_matrix * m_view_matrix;
	}

	void CameraComponent::inflate()
	{
		// get transform component
		m_transform_component = m_parent.lock()->getComponent(TransformComponent);

		// bind camera input callbacks
		m_key_event_handle = g_runtime_context.eventSystem()->addListener(EEventType::WindowKey, 
			std::bind(&CameraComponent::onKey, this, std::placeholders::_1));
		m_cursor_pos_event_handle = g_runtime_context.eventSystem()->addListener(EEventType::WindowCursorPos, 
			std::bind(&CameraComponent::onCursorPos, this, std::placeholders::_1));
		m_scroll_event_handle = g_runtime_context.eventSystem()->addListener(EEventType::WindowScroll, 
			std::bind(&CameraComponent::onScroll, this, std::placeholders::_1));

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

		// update camera rotation
		float& yaw = m_transform_component->m_rotation.y;
		float& pitch = m_transform_component->m_rotation.z;
		yaw += xoffset;
		pitch -= yoffset;
		pitch = std::clamp(pitch, -89.0f, 89.0f);

		updatePose();
	}

	void CameraComponent::onScroll(const std::shared_ptr<class Event>& event)
	{
		if (!m_mouse_focused)
		{
			return;
		}

		const WindowScrollEvent* scroll_event = static_cast<const WindowScrollEvent*>(event.get());
		m_transform_component->m_position += m_forward * (float)scroll_event->yoffset * m_zoom_speed;
	}

	void CameraComponent::updatePose()
	{
		m_forward = m_transform_component->getForwardVector();
		m_right = glm::cross(m_forward, k_up_vector);
		m_up = glm::cross(m_right, m_forward);
	}

	glm::mat4 CameraComponent::getProjectionMatrixYInverted(const glm::mat4& projection_matrix)
	{
		glm::mat4 projection_matrix_y_inverted = projection_matrix;
		projection_matrix_y_inverted[1][1] *= -1.0f;
		return projection_matrix_y_inverted;
	}

}