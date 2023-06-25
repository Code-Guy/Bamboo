#include "shader_manager.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include <array>

namespace Bamboo
{
	void ShaderManager::init()
	{
		// compile all shaders that have been modified

	}

	void ShaderManager::destroy()
	{
		for (const auto& iter : m_shader_modules)
		{
			vkDestroyShaderModule(VulkanRHI::get().getDevice(), iter.second, nullptr);
		}
	}

	VkShaderModule ShaderManager::getShaderModule(const std::string& name)
	{
		if (m_shader_modules.find(name) != m_shader_modules.end())
		{
			return m_shader_modules[name];
		}

		return VK_NULL_HANDLE;
	}

	std::string ShaderManager::execute(const char* cmd)
	{
		std::array<char, 128> buffer;
		std::string result;

#ifdef __linux__
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#elif _WIN32
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#endif

		if (!pipe)
		{
			LOG_FATAL("popen failed!");
			return "";
		}

		while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) 
		{
			result += buffer.data();
		}
		return result;
	}

}