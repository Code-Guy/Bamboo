#include "config_manager.h"

namespace Bamboo
{
	void ConfigManager::init(const std::string& config_file_name)
	{
		m_config_node = YAML::LoadFile(config_file_name);

	}

	int ConfigManager::getWindowWidth()
	{
		return m_config_node["window"]["width"].as<int>();
	}

	int ConfigManager::getWindowHeight()
	{
		return m_config_node["window"]["height"].as<int>();
	}

	std::string ConfigManager::getWindowTitle()
	{
		return m_config_node["window"]["title"].as<std::string>();
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