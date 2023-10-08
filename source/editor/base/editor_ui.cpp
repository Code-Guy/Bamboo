#include "editor_ui.h"
#include "engine/function/render/window_system.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>

namespace Bamboo
{
	void ImGuiImage::destroy()
	{
		if (is_from_file)
		{
			image_view_sampler.destroy();
		}
		
		ImGui_ImplVulkan_RemoveTexture(tex_id);
	}

	void EditorUI::destroy()
	{
		for (auto& iter : m_imgui_images)
		{
			iter.second->destroy();
		}
		m_imgui_images.clear();
	}

	void EditorUI::updateWindowRegion()
	{
		uint32_t new_pos_x = ImGui::GetCursorScreenPos().x;
		uint32_t new_pos_y = ImGui::GetCursorScreenPos().y;
		if (m_content_region.x != new_pos_x || m_content_region.y != new_pos_y)
		{
			m_content_region.x = new_pos_x;
			m_content_region.y = new_pos_y;

			onWindowRepos();
		}

		ImVec2 m_new_size = ImGui::GetContentRegionAvail();
		uint32_t new_width = static_cast<uint32_t>(m_new_size.x);
		uint32_t new_height = static_cast<uint32_t>(m_new_size.y);
		if (m_content_region.z != new_width || m_content_region.w != new_height)
		{
			m_content_region.z = new_width;
			m_content_region.w = new_height;

			onWindowResize();
		}
	}

	std::shared_ptr<ImGuiImage> EditorUI::loadImGuiImageFromFile(const std::string& filename)
	{
		if (m_imgui_images.find(filename) != m_imgui_images.end())
		{
			return m_imgui_images[filename];
		}

		std::shared_ptr<ImGuiImage> image = std::make_shared<ImGuiImage>();
		image->image_view_sampler = VulkanUtil::loadImageViewSampler(filename);
		image->tex_id = ImGui_ImplVulkan_AddTexture(image->image_view_sampler.sampler, image->image_view_sampler.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		image->is_from_file = true;
		m_imgui_images[filename] = image;

		return image;
	}

	std::shared_ptr<ImGuiImage> EditorUI::loadImGuiImageFromTexture2D(std::shared_ptr<class Texture2D>& texture)
	{
		std::shared_ptr<ImGuiImage> image = std::make_shared<ImGuiImage>();
		image->image_view_sampler = texture->m_image_view_sampler;
		image->tex_id = ImGui_ImplVulkan_AddTexture(image->image_view_sampler.sampler, image->image_view_sampler.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		image->is_from_file = false;
		m_imgui_images[texture->getURL()] = image;

		return image;
	}

	ImFont* EditorUI::smallFont()
	{
		return ImGui::GetIO().Fonts->Fonts[1];
	}

	ImFont* EditorUI::bigIconFont()
	{
		return ImGui::GetIO().Fonts->Fonts[2];
	}

	bool EditorUI::isFocused()
	{
		return !isPoppingUp() && isMouseFocused();
	}

	bool EditorUI::isPoppingUp()
	{
		ImGuiContext& g = *GImGui;
		return !g.OpenPopupStack.empty();
	}

	bool EditorUI::isMouseFocused()
	{
		int xpos, ypos;
		g_engine.windowSystem()->getMousePos(xpos, ypos);
		return xpos > m_content_region.x && xpos < m_content_region.x + m_content_region.z &&
			ypos > m_content_region.y && ypos < m_content_region.y + m_content_region.w;
	}

}