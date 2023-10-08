#include "shader_manager.h"
#include "engine/core/vulkan/vulkan_rhi.h"
#include <array>

namespace Bamboo
{
	void ShaderManager::init()
	{
		// create spv dir if doesn't exist
		const auto& fs = g_engine.fileSystem();
		std::string spv_dir = fs->getSpvDir();
		if (!fs->exists(spv_dir))
		{
			fs->createDir(spv_dir);
		}

		// get compiled spv filename and modified time
		std::map<std::string, std::string> spv_basename_modified_time_map;
		std::vector<std::string> spv_filenames = fs->traverse(spv_dir);
		for (const std::string& spv_filename : spv_filenames)
		{
			if (fs->extension(spv_filename) != "spv")
			{
				continue;
			}

			std::string spv_basename = fs->basename(spv_filename); 
			std::vector<std::string> splits = StringUtil::split(spv_basename, "-");
			spv_basename_modified_time_map[splits[0]] = splits[1];
			m_shader_filenames[splits[0]] = spv_filename;
		}

		// if shader dir doesn't exist, means in game mode, skip compiling stage
		if (!fs->exists(fs->getShaderDir()))
		{
			return;
		}

		// get shader include directory
		bool need_compile_all = false;
		std::string global_shader_include_dir = fs->global(fs->combine(fs->getShaderDir(), std::string("include")));
		std::string shader_include_dir_modified_time = fs->modifiedTime(global_shader_include_dir);
		std::string spv_include_filename = fs->combine(spv_dir, std::string("include.txt"));
		if (!fs->exists(spv_include_filename))
		{
			need_compile_all = true;
			fs->writeString(spv_include_filename, shader_include_dir_modified_time);
		}
		else
		{
			std::string last_shader_include_dir_modified_time;
			fs->loadString(spv_include_filename, last_shader_include_dir_modified_time);
			if (shader_include_dir_modified_time != last_shader_include_dir_modified_time)
			{
				need_compile_all = true;
				fs->writeString(spv_include_filename, shader_include_dir_modified_time);
			}
		}

		// compile glsl shader if necessary
		std::vector<std::string> glsl_filenames = fs->traverse(fs->getShaderDir());
		auto removing_shader_filenames = m_shader_filenames;
		for (const std::string& glsl_filename : glsl_filenames)
		{
			if (fs->isDir(glsl_filename))
			{
				continue;
			}

			std::string glsl_basename = fs->filename(glsl_filename);
			std::string modified_time = fs->modifiedTime(glsl_filename);
			removing_shader_filenames.erase(glsl_basename);

			bool need_compile = spv_basename_modified_time_map.find(glsl_basename) == spv_basename_modified_time_map.end() ||
				modified_time != spv_basename_modified_time_map[glsl_basename] || need_compile_all;
			if (need_compile)
			{
				// remove old spv file
				fs->removeFile(m_shader_filenames[glsl_basename]);

				std::string global_glsl_filename = fs->global(glsl_filename);
				std::string spv_filename = StringUtil::format("%s/%s-%s.spv", spv_dir.c_str(), glsl_basename.c_str(), modified_time.c_str());
				std::string global_spv_filename = fs->global(spv_filename);
				std::string shader_compile_cmd = StringUtil::format("%s --target-env vulkan1.3 -I%s -g -o \"%s\" \"%s\"", 
					VULKAN_SHADER_COMPILER, global_shader_include_dir.c_str(), global_spv_filename.c_str(), global_glsl_filename.c_str());
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

		// remove all spv files whose corresponding glsl file has been removed
		for (const auto& iter : removing_shader_filenames)
		{
			fs->removeFile(iter.second);
		}
	}

	void ShaderManager::destroy()
	{
		for (const auto& iter : m_shader_modules)
		{
			vkDestroyShaderModule(VulkanRHI::get().getDevice(), iter.second, nullptr);
		}
	}

	VkPipelineShaderStageCreateInfo ShaderManager::getShaderStageCI(const std::string& name, VkShaderStageFlagBits stage)
	{
		if (m_shader_filenames.find(name) == m_shader_filenames.end())
		{
			LOG_FATAL("failed to find shader {}", name);
			return {};
		}

		VkShaderModule shader_module;
		if (m_shader_modules.find(name) != m_shader_modules.end())
		{
			shader_module = m_shader_modules[name];
		}
		else
		{
			// load spv binary data
			std::vector<uint8_t> code;
			g_engine.fileSystem()->loadBinary(m_shader_filenames[name], code);

			// create shader module
			VkShaderModuleCreateInfo shader_module_ci{};
			shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_module_ci.codeSize = code.size();
			shader_module_ci.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkResult result = vkCreateShaderModule(VulkanRHI::get().getDevice(), &shader_module_ci, nullptr, &shader_module);
			CHECK_VULKAN_RESULT(result, "create shader module");
			m_shader_modules[name] = shader_module;
		}
		
		// create shader stage create info
		VkPipelineShaderStageCreateInfo shader_stage_ci{};
		shader_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_ci.stage = stage;
		shader_stage_ci.module = shader_module;
		shader_stage_ci.pName = "main";
		shader_stage_ci.pSpecializationInfo = nullptr;

		return shader_stage_ci;
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