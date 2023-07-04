#pragma once

#include <map>
#include <imgui/imgui.h>
#include <imgui/font/IconsFontAwesome5.h>
#include "runtime/core/vulkan/vulkan_util.h"

namespace Bamboo
{
	struct ImGuiImage
	{
		uint32_t width;
		uint32_t height;
		uint32_t channels;

		VmaImageViewSampler image_view_sampler;
		VkDescriptorSet desc_set;

		void destroy();
	};

	class EditorUI
	{
	public:
		virtual void init() = 0;
		virtual void construct() = 0;
		virtual void destroy();
		virtual void onWindowResize() {}

	protected:
		bool checkWindowResize();
		std::string combine(const char* c, const std::string& s);
		std::shared_ptr<ImGuiImage> loadImGuiImage(const std::string& filename);

		std::string m_title;
		uint32_t m_width = 0, m_height = 0;

	private:
		std::map<std::string, std::shared_ptr<ImGuiImage>> m_imgui_images;
	};
}