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
		void constructComponent(const std::shared_ptr<class Component>& component);
		void onSelectEntity(const std::shared_ptr<class Event>& event);

		std::shared_ptr<class Entity> m_selected_entity;
	};
}