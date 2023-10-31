#pragma once

#include <map>
#include <imgui/imgui.h>
#include <imgui/font/IconsFontAwesome5.h>
#include <glm/glm.hpp>

#include "engine/resource/asset/texture_2d.h"

namespace Bamboo
{
	struct ImGuiImage
	{
		VmaImageViewSampler image_view_sampler;
		VkDescriptorSet tex_id;

		bool is_owned;

		void destroy();
	};

	class EditorUI
	{
	public:
		virtual void init();
		virtual void construct() = 0;
		virtual void destroy();
		virtual void onWindowResize() {}
		virtual void onWindowRepos() {}

	protected:
		void updateWindowRegion();
		std::shared_ptr<ImGuiImage> loadImGuiImageFromFile(const std::string& filename);
		std::shared_ptr<ImGuiImage> loadImGuiImageFromTexture2D(std::shared_ptr<class Texture2D>& texture);
		std::shared_ptr<ImGuiImage> loadImGuiImageFromImageViewSampler(const VmaImageViewSampler& image_view_sampler);
		std::shared_ptr<ImGuiImage> getImGuiImageFromCache(const URL& url);
		ImFont* defaultFont();
		ImFont* smallFont();
		ImFont* bigIconFont();

		bool isFocused();
		bool isPoppingUp();
		bool isImGuiImageLoaded(const URL& url);

		std::string m_title;
		char m_title_buf[128];
		glm::uvec4 m_content_region;

	private:
		bool isMouseFocused();

		VkSampler m_texture_2d_sampler;
		std::map<URL, std::shared_ptr<ImGuiImage>> m_imgui_images;
	};
}