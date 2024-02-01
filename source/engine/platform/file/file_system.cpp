#include "engine/core/base/macro.h"
#include <fstream>
#include <algorithm>

namespace Bamboo
{
	void FileSystem::init()
	{
		std::vector<std::string> headers = { "./", "../", "../../" };
		for (const std::string& header : headers)
		{
			if (std::filesystem::exists(std::filesystem::path(header + "asset")))
			{
				m_header = header;
			}
		}
		if (m_header.empty())
		{
			LOG_FATAL("failed to find engine asset");
		}

		// create log and cache folder
		std::string log_dir = getLogDir();
		if (!exists(log_dir))
		{
			createDir(log_dir);
		}

		std::string cache_dir = getCacheDir();
		if (!exists(cache_dir))
		{
			createDir(cache_dir);
		}
	}

	void FileSystem::destroy()
	{

	}

	std::string FileSystem::absolute(const std::string& path)
	{
		std::filesystem::path header = m_header;
		return header.append(path).generic_string();
	}

	std::string FileSystem::global(const std::string& path)
	{
		return std::filesystem::absolute(path).generic_string();
	}

	std::string FileSystem::relative(const std::string& path)
	{
		std::string rel_path = std::filesystem::relative(path, m_header).generic_string();
		return rel_path;
	}

	std::string FileSystem::extension(const std::string& path)
	{
		std::string extension = std::filesystem::path(path).extension().generic_string();
		if (!extension.empty() && extension[0] == '.')
		{
			extension.erase(0, 1);
		}
		return extension;
	}

	std::string FileSystem::basename(const std::string& path)
	{
		return std::filesystem::path(path).stem().generic_string();
	}

	std::string FileSystem::filename(const std::string& path)
	{
		return std::filesystem::path(path).filename().generic_string();
	}

	std::string FileSystem::dir(const std::string& path)
	{
		return std::filesystem::path(path).parent_path().generic_string();
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
					last_write_time = std::max(last_write_time, (long long)std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(file).time_since_epoch()).count());
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
				filenames.push_back(file.path().generic_string());
			}
		}
		else
		{
			for (const auto& file : std::filesystem::directory_iterator(path))
			{
				filenames.push_back(file.path().generic_string());
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

	std::string FileSystem::validateBasename(const std::string& basename)
	{
		std::vector<char> invalid_chars = {
			'/', ':', '*', '?', '"', '<', '>', '|'
		};

		std::string validated_basename;
		for (size_t i = 0; i < basename.size(); ++i)
		{
			bool is_valid_char = std::find(invalid_chars.begin(), invalid_chars.end(), basename[i]) == invalid_chars.end();
			validated_basename.push_back(is_valid_char ? basename[i] : '_');
		}
		return validated_basename;
	}

	std::string FileSystem::getAssetDir()
	{
		return absolute("asset");
	}

	std::string FileSystem::getShaderDir()
	{
		return combine(std::string(BAMBOO_DIR), std::string("shader"));
	}

	std::string FileSystem::getSpvDir()
	{
		return absolute("asset/engine/shader");
	}

	std::string FileSystem::getLogDir()
	{
		return absolute("log");
	}

	std::string FileSystem::getCacheDir()
	{
		return absolute("cache");
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

	void FileSystem::copyFile(const std::string& from, const std::string& to)
	{
		std::filesystem::copy(from, to, std::filesystem::copy_options::overwrite_existing);
	}

	void FileSystem::renameFile(const std::string& dir, const std::string& old_name, const std::string& new_name)
	{
		if (old_name.compare(new_name))
		{
			try
			{
				std::filesystem::rename(dir + old_name, dir + new_name);
			}
			catch (const std::filesystem::filesystem_error& e)
			{
				LOG_WARNING("rename file error: {}", e.what());
			}
		}
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