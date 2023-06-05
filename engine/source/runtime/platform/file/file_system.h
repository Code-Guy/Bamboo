#pragma once

#include <filesystem>

namespace Bamboo
{
	class FileSystem
	{
	public:
		void init();
		void destroy();

		std::string absolute(const std::string& path);

		std::string extension(const std::string& path);
		std::string basename(const std::string& path);
		std::string filename(const std::string& path);
		std::string dir(const std::string& path);

		bool exists(const std::string& path);
		bool create(const std::string& path);

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