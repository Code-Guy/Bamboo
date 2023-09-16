#pragma once

#include "component.h"
#include "runtime/core/math/transform.h"

namespace Bamboo
{
	enum class EProjectionType
	{
		Perspective, Orthographic
	};

	class CameraComponent : public Component
	{
	public:
		CameraComponent();

		glm::vec3 getPosition();
		glm::mat4 getViewMatrix();
		glm::mat4 getProjectionMatrix();
		glm::mat4 getProjectionMatrix(EProjectionType projection_type);
		glm::mat4 getViewProjectionMatrix();

		glm::mat4 getViewMatrixNoTranslation();
		glm::mat4 getProjectionMatrixNoInverted();

		std::shared_ptr<class TransformComponent> getTransformComponent() { return m_transform_component; }
		void setInput(bool mouse_right_button_pressed, bool mouse_focused);

		virtual void tick(float delta_time) override;
		virtual void inflate() override;

		// projection
		EProjectionType m_projection_type;
		float m_fovy;
		float m_aspect_ratio;
		float m_near;
		float m_far;
		float m_ortho_width;

		// movement
		float m_move_speed;
		float m_turn_speed;
		float m_zoom_speed;

		// postprocessing
		float m_exposure;

	private:
		REGISTER_REFLECTION(Component)

		template<class Archive>
		void serialize(Archive& ar)
		{
			// component
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));

			// projection
			ar(cereal::make_nvp("fovy", m_fovy));
			ar(cereal::make_nvp("near", m_near));
			ar(cereal::make_nvp("far", m_far));

			// movement
			ar(cereal::make_nvp("move_speed", m_move_speed));
			ar(cereal::make_nvp("turn_speed", m_turn_speed));
			ar(cereal::make_nvp("zoom_speed", m_zoom_speed));

			// postprocessing
			ar(cereal::make_nvp("exposure", m_exposure));
		}

		void onKey(const std::shared_ptr<class Event>& event);
		void onCursorPos(const std::shared_ptr<class Event>& event);
		void onScroll(const std::shared_ptr<class Event>& event);

		void updatePose();
		void invertProjectionMatrix(glm::mat4& proj);

		// camera component depends a transform component
		std::shared_ptr<class TransformComponent> m_transform_component;

		glm::vec3 m_forward;
		glm::vec3 m_right;
		glm::vec3 m_up;
		glm::vec3 m_last_rotation;

		bool m_move_forward = false;
		bool m_move_back = false;
		bool m_move_left = false;
		bool m_move_right = false;
		bool m_move_up = false;
		bool m_move_down = false;

		bool m_mouse_right_button_pressed = false;
		bool m_mouse_focused = false;

		double m_last_xpos = 0.0f, m_last_ypos = 0.0f;
	};
}