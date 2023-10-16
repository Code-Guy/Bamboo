#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace Bamboo
{
	class ConfigManager
	{
	public:
		void init();
		void destroy() {}

		int getWindowWidth();
		int getWindowHeight();
		bool isFullscreen();

		std::string getDefaultWorldUrl();
		std::string getEditorLayout();
		bool getSaveLayout();
		
		bool isEditor();

	private:
		YAML::Node m_config_node;
	};
}