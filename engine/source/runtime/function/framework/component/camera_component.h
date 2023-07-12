#pragma once

#include "component.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "runtime/core/math/transform.h"

namespace Bamboo
{
	class CameraComponent : public Component
	{
	public:
		CameraComponent();

		void setContentRegion(const glm::uvec4& content_region);

		glm::mat4 getViewMatrix();
		glm::mat4 getPerspectiveMatrix();
		glm::mat4 getViewPerspectiveMatrix();

		virtual void tick(float delta_time) override;
		virtual void inflate() override;

		// projection
		float m_fovy;
		float m_aspect_ratio;
		float m_near;
		float m_far;

		// movement
		float m_move_speed;
		float m_turn_speed;
		float m_zoom_speed;

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			// component
			ar(cereal::make_nvp("component", cereal::base_class<Component>(this)));

			// projection
			ar(cereal::make_nvp("fovy", m_fovy));
			ar(cereal::make_nvp("aspect_ratio", m_aspect_ratio));
			ar(cereal::make_nvp("near", m_near));
			ar(cereal::make_nvp("far", m_far));

			// movement
			ar(cereal::make_nvp("move_speed", m_move_speed));
			ar(cereal::make_nvp("turn_speed", m_turn_speed));
			ar(cereal::make_nvp("zoom_speed", m_zoom_speed));
		}

		void onKey(const std::shared_ptr<class Event>& event);
		void onCursorPos(const std::shared_ptr<class Event>& event);
		void onMouseButton(const std::shared_ptr<class Event>& event);
		void onScroll(const std::shared_ptr<class Event>& event);

		void updatePose();
		bool isInContentRegion(double xpos, double ypos);

		// camera component depends a transform component
		std::shared_ptr<class TransformComponent> m_transform_component;

		glm::vec3 m_forward;
		glm::vec3 m_right;
		glm::vec3 m_up;

		bool m_move_forward;
		bool m_move_back;
		bool m_move_left;
		bool m_move_right;
		bool m_move_up;
		bool m_move_down;

		bool m_mouse_in_content;
		bool m_mouse_right_button_pressed;

		glm::uvec4 m_content_region;
		double m_last_xpos, m_last_ypos;
	};
}