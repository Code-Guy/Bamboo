#pragma once

#include "editor/base/editor_ui.h"
#include "runtime/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	enum class EOperationMode
	{
		Pick, Translate, Rotate, Scale
	};

	enum class ECoordinateMode
	{
		Local, World
	};

	class SimulationUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;
		virtual void onWindowResize() override;

	private:
		void loadAsset(const std::string& url);
		bool constructRadioButtonPopup(const std::string& popup_name, const std::vector<std::string>& values, int& index);
		void constructCheckboxPopup(const std::string& popup_name, std::vector<std::pair<std::string, bool>>& values);
		void constructOperationModeButtons();
		void constructImGuizmo();

		void onKey(const std::shared_ptr<class Event>& event);
		void onSelectEntity(const std::shared_ptr<class Event>& event);

		void handleDragDropTarget(const glm::vec2& mouse_pos, const glm::vec2& viewport_size);
		glm::vec3 calcPlacePos(const glm::vec2& mouse_pos, const glm::vec2& viewport_size);

		VkSampler m_color_texture_sampler;
		VkDescriptorSet m_color_texture_desc_set = VK_NULL_HANDLE;

		ECoordinateMode m_coordinate_mode;
		EOperationMode m_operation_mode;
		bool m_mouse_right_button_pressed;
		std::shared_ptr<class CameraComponent> m_camera_component;

		std::shared_ptr<class Entity> m_created_entity;
		std::weak_ptr<class Entity> m_selected_entity;
	};
}