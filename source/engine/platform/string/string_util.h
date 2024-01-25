#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

namespace Bamboo
{
	class StringUtil
	{
	public:
		static std::vector<std::string> split(const std::string& s, const std::string& delimiter);
		static bool replace(std::string& str, const std::string& from, const std::string& to);
		static void replace_all(std::string& str, const std::string& from, const std::string& to);
		static bool remove(std::string& str, const std::string& sub_str);

		static void ltrim(std::string& s);
		static void rtrim(std::string& s);
		static void trim(std::string& s);

		template<typename ... Args>
		static std::string format(const std::string& format, Args ... args)
		{
			int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
			if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
			auto size = static_cast<size_t>(size_s);
			std::unique_ptr<char[]> buf(new char[size]);
			std::snprintf(buf.get(), size, format.c_str(), args ...);
			return std::string(buf.get(), buf.get() + size - 1);
		}
	};
}