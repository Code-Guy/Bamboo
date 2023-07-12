#include "config_manager.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	void ConfigManager::init()
	{
		m_config_node = YAML::LoadFile(TO_ABSOLUTE("config/engine.yaml"));
	}

	int ConfigManager::getWindowWidth()
	{
		return m_config_node["window"]["width"].as<int>();
	}

	int ConfigManager::getWindowHeight()
	{
		return m_config_node["window"]["height"].as<int>();
	}

	std::string ConfigManager::getDefaultWorldUrl()
	{
		return m_config_node["default_world_url"].as<std::string>();
	}

	std::string ConfigManager::getEditorLayout()
	{
		return m_config_node["editor_layout"].as<std::string>();
	}

}