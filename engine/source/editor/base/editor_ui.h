#pragma once

#include <map>
#include <imgui/imgui.h>
#include <imgui/font/IconsFontAwesome5.h>
#include <glm/glm.hpp>

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
		virtual void onWindowRepos() {}

	protected:
		void updateWindowRegion();
		std::shared_ptr<ImGuiImage> loadImGuiImage(const std::string& filename);
		ImFont* smallFont();

		std::string m_title;
		char m_title_buf[128];
		glm::uvec4 m_content_region;

	private:
		std::map<std::string, std::shared_ptr<ImGuiImage>> m_imgui_images;
	};
}