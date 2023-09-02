#pragma once

#include "editor/base/editor_ui.h"
#include "runtime/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	enum class InteractiveMode
	{
		Select, Translate, Rotation, Scale
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
		void constructInteractiveModeButtons();

		VkSampler m_color_texture_sampler;
		VkDescriptorSet m_color_texture_desc_set = VK_NULL_HANDLE;
	};
}