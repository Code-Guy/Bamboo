#pragma once

#include "editor/base/editor_ui.h"

namespace Bamboo
{
	class WorldUI : public EditorUI
	{
	public:
		virtual void init() override;
		virtual void construct() override;
		virtual void destroy() override;

	private:
		void constructEntityTree(const std::shared_ptr<class Entity>& entity);
		void onSelectEntity(const std::shared_ptr<class Event>& event);

		uint32_t m_selected_entity_id = UINT_MAX;
	};
}