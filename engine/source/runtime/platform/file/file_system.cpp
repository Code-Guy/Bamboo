#include "runtime/core/base/macro.h"
#include <fstream>
#include <algorithm>

namespace Bamboo
{
	void FileSystem::init()
	{
		if (std::filesystem::exists(std::filesystem::path("asset")))
		{
			m_header = std::filesystem::path(".");
		}
		else if (std::filesystem::exists(std::filesystem::path("../../../../engine/asset")))
		{
			m_header = std::filesystem::path("../../../../engine/");
		}
		else if (std::filesystem::exists(std::filesystem::path("../../../../../engine/asset")))
		{
			m_header = std::filesystem::path("../../../../../engine/");
		}
		else
		{
			LOG_FATAL("failed to find engine asset");
		}
	}

	void FileSystem::destroy()
	{

	}

	std::string FileSystem::absolute(const std::string& path)
	{
		std::filesystem::path header = m_header;
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

	std::vector<std::string> FileSystem::traverse(const std::string& path, bool is_recursive, EFileOrderType file_order_type, bool is_reverse)
	{
		std::vector<std::string> filenames;

		if (is_recursive)
		{
			for (const auto& file : std::filesystem::recursive_directory_iterator(path))
			{
				filenames.push_back(file.path().string());
			}
		}
		else
		{
			for (const auto& file : std::filesystem::directory_iterator(path))
			{
				filenames.push_back(file.path().string());
			}
		}

		std::sort(filenames.begin(), filenames.end(), 
			[file_order_type, is_reverse](const std::string& lhs, const std::string& rhs)
			{
				bool result = false;
				switch (file_order_type)
				{
				case EFileOrderType::Name:
					result = lhs < rhs;
					break;
				case EFileOrderType::Time:
					result = std::filesystem::last_write_time(lhs) < std::filesystem::last_write_time(rhs);
					break;
				case EFileOrderType::Size:
					result = std::filesystem::file_size(lhs) < std::filesystem::file_size(rhs);
					break;
				default:
					break;
				}
				return is_reverse ? !result : result;
			});

		return filenames;
	}

	std::string FileSystem::asset_dir()
	{
		return absolute("asset");
	}

	bool FileSystem::exists(const std::string& path)
	{
		return std::filesystem::exists(path) || std::filesystem::exists(absolute(path));
	}

	bool FileSystem::create_file(const std::string& filename, std::ios_base::openmode mode)
	{
		if (exists(filename))
		{
			return false;
		}

		std::ofstream ofs(filename);
		ofs.close();
		return true;
	}

	bool FileSystem::create_dir(const std::string& path, bool is_recursive)
	{
		if (exists(path))
		{
			return false;
		}

		if (is_recursive)
		{
			return std::filesystem::create_directories(std::filesystem::path(path));
		}
		return std::filesystem::create_directory(std::filesystem::path(path));
	}

	bool FileSystem::remove_file(const std::string& filename)
	{
		if (!exists(filename))
		{
			return false;
		}

		return std::filesystem::remove(filename);
	}

	bool FileSystem::remove_dir(const std::string& path, bool is_recursive)
	{
		if (!exists(path))
		{
			return false;
		}

		if (is_recursive)
		{
			return std::filesystem::remove_all(path) > 0;
		}
		return std::filesystem::remove(path);
	}

}