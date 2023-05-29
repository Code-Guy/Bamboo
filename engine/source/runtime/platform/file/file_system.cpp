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

	std::string FileSystem::extension(const std::string& path)
	{
		std::string extension = std::filesystem::path(path).extension().string();
		if (!extension.empty() && extension[0] == '.')
		{
			extension.erase(0, 1);
		}
		return extension;
	}

	std::string FileSystem::basename(const std::string& path)
	{
		return std::filesystem::path(path).stem().string();
	}

	std::string FileSystem::filename(const std::string& path)
	{
		return std::filesystem::path(path).filename().string();
	}

	std::string FileSystem::dir(const std::string& path)
	{
		return std::filesystem::path(path).parent_path().string();
	}

}