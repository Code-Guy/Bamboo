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

	std::string FileSystem::global(const std::string& path)
	{
		return std::filesystem::absolute(path).string();
	}

	std::string FileSystem::relative(const std::string& path)
	{
		return std::filesystem::relative(path, m_header).string();
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

	std::string FileSystem::modifiedTime(const std::string& path)
	{
		return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(path).time_since_epoch()).count());
	}

	std::vector<std::string> FileSystem::traverse(const std::string& path, bool is_recursive, EFileOrderType file_order_type, bool is_reverse)
	{
		std::vector<std::string> filenames;
		if (!std::filesystem::exists(path))
		{
			return filenames;
		}

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

	std::string FileSystem::getAssetDir()
	{
		return absolute("asset");
	}

	std::string FileSystem::getShaderDir()
	{
		return absolute("shader");
	}

	std::string FileSystem::getSpvDir()
	{
		return absolute("asset/engine/spv");
	}

	bool FileSystem::exists(const std::string& path)
	{
		return std::filesystem::exists(path) || std::filesystem::exists(absolute(path));
	}

	bool FileSystem::isFile(const std::string& path)
	{
		return std::filesystem::is_regular_file(path);
	}

	bool FileSystem::isDir(const std::string& path)
	{
		return std::filesystem::is_directory(path);
	}

	bool FileSystem::isEmptyDir(const std::string& path)
	{
		return std::filesystem::path(path).empty();
	}

	bool FileSystem::createFile(const std::string& filename, std::ios_base::openmode mode)
	{
		if (exists(filename))
		{
			return false;
		}

		std::ofstream ofs(filename);
		ofs.close();
		return true;
	}

	bool FileSystem::createDir(const std::string& path, bool is_recursive)
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

	bool FileSystem::removeFile(const std::string& filename)
	{
		if (!exists(filename))
		{
			return false;
		}

		return std::filesystem::remove(filename);
	}

	bool FileSystem::removeDir(const std::string& path, bool is_recursive)
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

	std::vector<char> FileSystem::loadBinary(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			LOG_FATAL("failed to load shader {}", filename);
			return {};
		}

		std::vector<char> buffer;
		size_t fileSize = (size_t)file.tellg();
		buffer.resize(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

}