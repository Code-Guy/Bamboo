#pragma once

#include <string>
#include <memory>
#include <imgui/imgui.h>

namespace Bamboo
{
	class EditorUI
	{
	public:
		virtual void construct() = 0;

	protected:
		std::string m_title;

	private:
		
	};
}