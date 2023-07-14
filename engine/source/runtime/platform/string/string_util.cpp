#include "string_util.h"

namespace Bamboo
{
	std::vector<std::string> StringUtil::split(const std::string& s, const std::string& delimiter)
	{
		std::vector<std::string> output;
		std::string::size_type prev_pos = 0, pos = 0;

		while ((pos = s.find(delimiter, pos)) != std::string::npos)
		{
			std::string substring(s.substr(prev_pos, pos - prev_pos));
			output.push_back(substring);

			prev_pos = ++pos;
		}

		output.push_back(s.substr(prev_pos, pos - prev_pos));
		return output;
	}

	bool StringUtil::replace(std::string& str, const std::string& from, const std::string& to)
	{
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
		{
			return false;
		}

		str.replace(start_pos, from.length(), to);
		return true;
	}

	void StringUtil::replace_all(std::string& str, const std::string& from, const std::string& to)
	{
		if (from.empty())
		{
			return;
		}

		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}

	bool StringUtil::remove(std::string& str, const std::string& sub_str)
	{
		std::size_t pos = str.find(sub_str);
		if (pos != std::string::npos)
		{
			str.erase(pos, sub_str.length());
			return true;
		}
		return false;
	}

	void StringUtil::ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	void StringUtil::rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	void StringUtil::trim(std::string& s)
	{
		rtrim(s);
		ltrim(s);
	}

}