#include "shader_manager.h"
#include "runtime/core/vulkan/vulkan_rhi.h"
#include <array>

namespace Bamboo
{
	void ShaderManager::init()
	{
		// create spv dir if doesn't exist
		const auto& fs = g_runtime_context.fileSystem();
		std::string spv_dir = fs->getSpvDir();
		if (!fs->exists(spv_dir))
		{
			fs->createDir(spv_dir);
		}

		// compile all shaders that have been modified
		std::vector<std::string> glsl_filenames = fs->traverse(fs->getShaderDir());
		std::vector<std::string> spv_filenames = fs->traverse(spv_dir);

		// get compiled spv filename and modified time
		std::map<std::string, std::string> spv_basename_modified_time_map;
		for (const std::string& spv_filename : spv_filenames)
		{
			std::string spv_basename = fs->basename(spv_filename); 
			std::vector<std::string> splits = StringUtil::split(spv_basename, "-");
			spv_basename_modified_time_map[splits[0]] = splits[1];
			m_shader_filenames[splits[0]] = spv_filename;
		}

		// compile glsl shader if necessary
		for (const std::string& glsl_filename : glsl_filenames)
		{
			std::string glsl_basename = fs->filename(glsl_filename);
			std::string modified_time = fs->modifiedTime(glsl_filename);

			bool need_compile = spv_basename_modified_time_map.find(glsl_basename) == spv_basename_modified_time_map.end() ||
				modified_time != spv_basename_modified_time_map[glsl_basename];
			if (need_compile)
			{
				// remove old spv file
				fs->removeFile(m_shader_filenames[glsl_basename]);

				std::string global_glsl_filename = fs->global(glsl_filename);
				std::string spv_filename = StringUtil::format("%s/%s-%s.spv", spv_dir.c_str(), glsl_basename.c_str(), modified_time.c_str());
				std::string global_spv_filename = fs->global(spv_filename);
				std::string shader_compile_cmd = StringUtil::format("%s --target-env vulkan1.3 -g -o \"%s\" \"%s\"", 
					VULKAN_SHADER_COMPILER, global_spv_filename.c_str(), global_glsl_filename.c_str());
				std::string result = execute(shader_compile_cmd.c_str());
				StringUtil::trim(result);
				if (!result.empty())
				{
					LOG_INFO("finished compiling shader {}, result: {}", glsl_basename, result);
				}
				else
				{
					LOG_INFO("finished compiling shader {}", glsl_basename);
				}

				m_shader_filenames[glsl_basename] = spv_filename;
			}
		}
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
		if (m_shader_filenames.find(name) == m_shader_filenames.end())
		{
			LOG_FATAL("failed to find shader {}", name);
			return VK_NULL_HANDLE;
		}

		if (m_shader_modules.find(name) != m_shader_modules.end())
		{
			return m_shader_modules[name];
		}

		// load spv binary data
		std::vector<char> code = g_runtime_context.fileSystem()->loadBinary(m_shader_filenames[name]);

		// create shader module
		VkShaderModuleCreateInfo shader_module_ci{};
		shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_ci.codeSize = code.size();
		shader_module_ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		VkResult result = vkCreateShaderModule(VulkanRHI::get().getDevice(), &shader_module_ci, nullptr, &shader_module);
		CHECK_VULKAN_RESULT(result, "create shader module");
		m_shader_modules[name] = shader_module;

		return shader_module;
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
			LOG_FATAL("failed to run command: {}", cmd);
			return "";
		}

		while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) 
		{
			result += buffer.data();
		}
		return result;
	}

}