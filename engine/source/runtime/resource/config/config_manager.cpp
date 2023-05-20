#include "config_manager.h"

namespace Bamboo
{
	void ConfigManager::init(const std::string& config_file_name)
	{
		config_node = YAML::LoadFile(config_file_name);

	}

	int ConfigManager::getWindowWidth()
	{
		return config_node["window"]["width"].as<int>();
	}

	int ConfigManager::getWindowHeight()
	{
		return config_node["window"]["height"].as<int>();
	}

	std::string ConfigManager::getWindowTitle()
	{
		return config_node["window"]["title"].as<std::string>();
	}

}