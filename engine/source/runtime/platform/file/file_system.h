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

	private:
		std::filesystem::path header;
	};
}