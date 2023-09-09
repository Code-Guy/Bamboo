#include "world_ui.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"

namespace Bamboo
{

	void WorldUI::init()
	{
		m_title = "World";

		g_runtime_context.eventSystem()->addListener(EEventType::SelectEntity, std::bind(&WorldUI::onSelectEntity, this, std::placeholders::_1));
	}

	void WorldUI::construct()
	{
		sprintf(m_title_buf, "%s %s###%s", ICON_FA_GLOBE, m_title.c_str(), m_title.c_str());
		if (!ImGui::Begin(m_title_buf))
		{
			ImGui::End();
			return;
		}

		// get current active world
		std::shared_ptr<World> current_world = g_runtime_context.worldManager()->getCurrentWorld();

		// traverse all entities
		const float k_unindent_w = 16;
		ImGui::Unindent(k_unindent_w);
		const auto& entities = current_world->getEntities();
		for (const auto& iter : entities)
		{
			const auto& entity = iter.second;
			if (entity->isRoot())
			{
				constructEntityTree(entity);
			}
		}
		
		ImGui::End();
	}

	void WorldUI::destroy()
	{
		EditorUI::destroy();

	}

	void WorldUI::constructEntityTree(const std::shared_ptr<class Entity>& entity)
	{
		uint32_t entity_id = entity->getID();

		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
		if (entity->isLeaf())
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
		}
		if (entity_id == m_selected_entity_id)
		{
			tree_node_flags |= ImGuiTreeNodeFlags_Selected;
		}
		
		ImGui::TreeNodeEx(entity->getName().c_str(), tree_node_flags);
		if (m_selected_entity_id != entity_id && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			g_runtime_context.eventSystem()->syncDispatch(std::make_shared<SelectEntityEvent>(entity_id));
		}

		for (const auto& child : entity->getChildren())
		{
			constructEntityTree(child.lock());
		}

		ImGui::TreePop();
	}

	void WorldUI::onSelectEntity(const std::shared_ptr<class Event>& event)
	{
		const SelectEntityEvent* p_event = static_cast<const SelectEntityEvent*>(event.get());
		m_selected_entity_id = p_event->entity_id;
	}

}