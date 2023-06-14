#pragma once

#include "editor_ui.h"

namespace Bamboo
{
	class AssetUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:

	};
}