#pragma once

#include "engine/platform/string/string_util.h"
#include <filesystem>

namespace Bamboo
{
	enum class EFileOrderType
	{
		Name, Time, Size
	};

	class FileSystem
	{
	public:
		void init();
		void destroy();

		std::string absolute(const std::string& path);
		std::string global(const std::string& path);
		std::string relative(const std::string& path);

		std::string extension(const std::string& path);
		std::string basename(const std::string& path);
		std::string filename(const std::string& path);
		std::string dir(const std::string& path);
		std::string modifiedTime(const std::string& path);
		std::vector<std::string> traverse(const std::string& path, bool is_recursive = false, 
			EFileOrderType file_order_type = EFileOrderType::Name, bool is_reverse = false);
		std::string validateBasename(const std::string& basename);

		std::string getAssetDir();
		std::string getShaderDir();
		std::string getSpvDir();

		bool exists(const std::string& path);
		bool isFile(const std::string& path);
		bool isDir(const std::string& path);
		bool isEmptyDir(const std::string& path);

		bool createFile(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out);
		bool createDir(const std::string& path, bool is_recursive = false);
		bool removeFile(const std::string& filename);
		bool removeDir(const std::string& path, bool is_recursive = false);

		bool loadBinary(const std::string& filename, std::vector<uint8_t>& data);
		bool writeString(const std::string& filename, const std::string& str);
		bool loadString(const std::string& filename, std::string& str);

		template<typename T, typename... Ts>
		std::string combine(const T& first, const Ts&... rest)
		{
			return std::filesystem::path(first).append(combine(rest...)).string();
		}

		template<typename T>
		T combine(const T& t)
		{
			return t;
		}

		template<typename ... Args>
		std::string format(const std::string& format, Args ... args)
		{
			int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
			if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
			auto size = static_cast<size_t>(size_s);
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format.c_str(), args ...);
			return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
		}

	private:
		std::filesystem::path m_header;
	};
}