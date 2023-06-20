#pragma once

#include "editor/base/editor_ui.h"

namespace Bamboo
{
	class PropertyUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:

	};
}