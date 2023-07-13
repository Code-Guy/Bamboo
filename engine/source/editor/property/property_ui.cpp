#include "property_ui.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"

namespace Bamboo
{

	void PropertyUI::init()
	{
		m_title = "Property";

		g_runtime_context.eventSystem()->addListener(EventType::UISelectEntity, std::bind(&PropertyUI::onSelectEntity, this, std::placeholders::_1));
	}

	void PropertyUI::construct()
	{
		std::string entity_name = m_selected_entity ? m_selected_entity->getName() : "";
		sprintf(m_title_buf, "%s %s###%s", ICON_FA_STREAM, (entity_name.empty() ? m_title : entity_name).c_str(), m_title.c_str());
		if (!ImGui::Begin(m_title_buf))
		{
			ImGui::End();
			return;
		}

		if (m_selected_entity)
		{
			for (const auto& component : m_selected_entity->getComponents())
			{
				constructComponent(component);
			}
		}

		ImGui::End();
	}

	void PropertyUI::destroy()
	{
		m_selected_entity.reset();
		EditorUI::destroy();

	}

	void PropertyUI::constructComponent(const std::shared_ptr<class Component>& component)
	{
		// construct name title
		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
		ImGui::TreeNodeEx(component->getTypeName().c_str(), tree_node_flags);

// 		for (auto& prop : rttr::type::get(b).get_properties())
// 		{
// 			std::cout << "name: " << prop.get_name() << std::endl;
// 			std::cout << "type: " << prop.get_type().get_name() << std::endl;
// 		}

		ImGui::TreePop();
	}

	void PropertyUI::onSelectEntity(const std::shared_ptr<class Event>& event)
	{
		const UISelectEntityEvent* p_event = static_cast<const UISelectEntityEvent*>(event.get());

		const auto& current_world = g_runtime_context.worldManager()->getCurrentWorld();
		m_selected_entity = current_world->getEntity(p_event->entity_id);
	}

}