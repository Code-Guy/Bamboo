#pragma once

#include <string>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>

namespace Bamboo
{
	struct URL
	{
	public:
		URL() = default;
		URL(const std::string& url): m_url(url)
		{
			toRelative();
		}

		URL(std::string&& url): m_url(std::move(url))
		{
			toRelative();
		}

		URL(const char* url) : m_url(std::string(url))
		{
			toRelative();
		}

		bool operator<(const URL& other) const
		{
			return m_url < other.m_url;
		}

		bool operator==(const URL& other) const
		{
			return m_url == other.m_url;
		}

		bool operator!=(const URL& other) const
		{
			return m_url != other.m_url;
		}

		std::string getAbsolute() const;
		std::string getBareName() const;
		std::string getFolder() const;

		bool empty() const;
		void clear();
		const std::string& str() const
		{
			return m_url;
		}

		static URL combine(const std::string& lhs, const std::string& rhs);

	private:
		friend class cereal::access;
		template<class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("url", m_url));
		}

		void toRelative();

		std::string m_url;
	};
}