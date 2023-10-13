#include "url.h"
#include "engine/core/base/macro.h"

namespace Bamboo
{
	std::string URL::getAbsolute() const
	{
		return TO_ABSOLUTE(m_url);
	}

	std::string URL::getBareName() const
	{
		std::string name = g_engine.fileSystem()->basename(m_url);
		std::string::size_type underline_pos = name.find_first_of('_');
		return name.substr(underline_pos + 1, name.length() - (underline_pos + 1));
	}

	std::string URL::getFolder() const
	{
		return g_engine.fileSystem()->dir(m_url);
	}

	bool URL::empty() const
	{
		return m_url.empty();
	}

	void URL::clear()
	{
		m_url.clear();
	}

	URL URL::combine(const std::string& lhs, const std::string& rhs)
	{
		return URL(g_engine.fileSystem()->combine(lhs, rhs));
	}

	void URL::toRelative()
	{
		if (!m_url.empty() && m_url[0] == '.')
		{
			m_url = g_engine.fileSystem()->relative(m_url);
		}
	}

}