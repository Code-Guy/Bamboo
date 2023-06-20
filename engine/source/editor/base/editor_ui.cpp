#include "editor_ui.h"
#include <imgui/backends/imgui_impl_vulkan.h>
#include <tinygltf/stb_image.h>

namespace Bamboo
{
	void ImGuiImage::destroy()
	{
		image_view_sampler.destroy();
		ImGui_ImplVulkan_RemoveTexture(desc_set);
	}

	void EditorUI::construct()
	{
		handleWindowResize();
	}

	void EditorUI::destroy()
	{
		for (auto& iter : m_imgui_images)
		{
			iter.second->destroy();
		}
		m_imgui_images.clear();
	}

	bool EditorUI::handleWindowResize()
	{
		ImVec2 m_new_size = ImGui::GetContentRegionAvail();
		uint32_t new_width = static_cast<uint32_t>(m_new_size.x);
		uint32_t new_height = static_cast<uint32_t>(m_new_size.y);

		if (m_width != new_width || m_height != new_height)
		{
			m_width = new_width;
			m_height = new_height;

			onWindowResize();
			return true;
		}

		return false;
	}

	std::string EditorUI::combine(const char* c, const std::string& s)
	{
		return std::string(c) + " " + s;
	}

	std::shared_ptr<ImGuiImage> EditorUI::loadImGuiImage(const std::string& filename)
	{
		if (m_imgui_images.find(filename) != m_imgui_images.end())
		{
			return m_imgui_images[filename];
		}

		std::shared_ptr<ImGuiImage> image = std::make_shared<ImGuiImage>();
		image->channels = 4;

		uint8_t* image_data = stbi_load(filename.c_str(), (int*)&image->width, (int*)&image->height, 0, image->channels);
		ASSERT(image_data != nullptr, "failed to load imgui image");

		createImageViewSampler(image->width, image->height, image_data, 1, true,
			VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, image->image_view_sampler);

		image->desc_set = ImGui_ImplVulkan_AddTexture(image->image_view_sampler.sampler, image->image_view_sampler.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_imgui_images[filename] = image;

		return image;
	}

}