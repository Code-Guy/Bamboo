#pragma once

#include <filesystem>

namespace Bamboo
{
	class FileSystem
	{
	public:
		void init();
		void destroy();

		std::string redirect(const std::string& path);
		std::string extension(const std::string& path);
		std::string basename(const std::string& path);
		std::string filename(const std::string& path);
		std::string dir(const std::string& path);

	private:
		std::filesystem::path header;
	};
}