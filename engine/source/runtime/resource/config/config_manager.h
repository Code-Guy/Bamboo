#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace Bamboo
{
	class ConfigManager
	{
	public:
		void init(const std::string& config_file_name);
		void destroy() {}

		int getWindowWidth();
		int getWindowHeight();
		std::string getWindowTitle();

	private:
		YAML::Node config_node;
	};
}