#pragma once

#include "runtime/core/vulkan/vulkan_util.h"
#include <map>

namespace Bamboo
{
	class ShaderManager
	{
	public:
		void init();
		void destroy();

		VkShaderModule getShaderModule(const std::string& name);

	private:
		std::string execute(const char* cmd);

		std::map<std::string, VkShaderModule> m_shader_modules;
	};
}