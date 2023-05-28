#include "file_system.h"
#include "runtime/core/log/log_system.h"
#include "runtime/core/base/macro.h"

namespace Bamboo
{
	void FileSystem::init()
	{
		if (std::filesystem::exists(std::filesystem::path("asset")))
		{
			header = std::filesystem::path(".");
		}
		else if (std::filesystem::exists(std::filesystem::path("../../../../engine/asset")))
		{
			header = std::filesystem::path("../../../../engine/");
		}
		else if (std::filesystem::exists(std::filesystem::path("../../../../../engine/asset")))
		{
			header = std::filesystem::path("../../../../../engine/");
		}
		else
		{
			LOG_FATAL("failed to find engine asset");
		}
	}

	void FileSystem::destroy()
	{

	}

	std::string FileSystem::redirect(const std::string& path)
	{
		return header.append(path).string();
	}
}