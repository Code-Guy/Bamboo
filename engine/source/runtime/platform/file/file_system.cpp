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
		std::string rel_path = std::filesystem::relative(path, m_header).string();
		StringUtil::replace_all(rel_path, "\\", "/");
		return rel_path;
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
		if (isFile(path))
		{
			return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(path).time_since_epoch()).count());
		}

		if (isDir(path))
		{
			long long last_write_time = 0;
			std::vector<std::string> files = traverse(path, true);
			for (const std::string& file : files)
			{
				if (isFile(file))
				{
					last_write_time = std::max(last_write_time, std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(file).time_since_epoch()).count());
				}
			}
			return std::to_string(last_write_time);
		}
		return "";
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

		std::ofstream ofs(filename, mode);
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

	bool FileSystem::loadBinary(const std::string& filename, std::vector<uint8_t>& data)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			LOG_FATAL("failed to load binary file {}", filename);
			return false;
		}

		size_t file_size = (size_t)file.tellg();
		data.resize(file_size);

		file.seekg(0);
		file.read((char*)data.data(), file_size);
		file.close();

		return true;
	}

	bool FileSystem::writeString(const std::string& filename, const std::string& str)
	{
		std::ofstream file(filename);
		if (!file.is_open())
		{
			LOG_FATAL("failed to write string file {}", filename);
			return false;
		}

		file << str;
		file.close();

		return true;
	}

	bool FileSystem::loadString(const std::string& filename, std::string& str)
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
			LOG_FATAL("failed to load string file {}", filename);
			return false;
		}

		file >> str;
		file.close();

		return true;
	}

}