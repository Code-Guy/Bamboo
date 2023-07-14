#include "property_ui.h"
#include "runtime/core/event/event_system.h"
#include "runtime/function/framework/world/world_manager.h"

namespace Bamboo
{

	void PropertyUI::init()
	{
		m_title = "Property";
		m_property_constructors = {
			{ EPropertyValueType::Bool, std::bind(&PropertyUI::constructPropertyBool, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Integar, std::bind(&PropertyUI::constructPropertyIntegar, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Float, std::bind(&PropertyUI::constructPropertyFloat, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::String, std::bind(&PropertyUI::constructPropertyString, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Vec2, std::bind(&PropertyUI::constructPropertyVec2, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Vec3, std::bind(&PropertyUI::constructPropertyVec3, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Vec4, std::bind(&PropertyUI::constructPropertyVec4, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Asset, std::bind(&PropertyUI::constructPropertyAsset, this, std::placeholders::_1, std::placeholders::_2) },
		};

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
			ImGuiTableFlags table_flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable;
			if (ImGui::BeginTable("components", 2, table_flags))
			{
				ImGui::TableSetupColumn("column_0", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				ImGui::TableSetupColumn("column_1", ImGuiTableColumnFlags_WidthStretch);

				for (const auto& component : m_selected_entity->getComponents())
				{
					constructComponent(component);
				}
				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	void PropertyUI::destroy()
	{
		m_selected_entity.reset();
		EditorUI::destroy();

	}

	void PropertyUI::onSelectEntity(const std::shared_ptr<class Event>& event)
	{
		const UISelectEntityEvent* p_event = static_cast<const UISelectEntityEvent*>(event.get());

		const auto& current_world = g_runtime_context.worldManager()->getCurrentWorld();
		m_selected_entity = current_world->getEntity(p_event->entity_id);
	}

	void PropertyUI::constructComponent(const std::shared_ptr<class Component>& component)
	{
		// add name title
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGuiTreeNodeFlags tree_node_flags = 0;
		tree_node_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | 
			ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap;

		const std::string& type_name = component->getTypeName();
		std::string title = type_name.substr(0, type_name.length() - 9);
		bool is_tree_open = ImGui::TreeNodeEx(title.c_str(), tree_node_flags);

		// add option button
		ImGui::TableNextColumn();
		ImVec4 rect_color = ImGui::IsItemActive() ? ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive) : (
			ImGui::IsItemHovered() ? ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered) : ImGui::GetStyleColorVec4(ImGuiCol_Header)
			);
		ImVec2 p_min = ImGui::GetCursorScreenPos();
		ImVec2 p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x, p_min.y + ImGui::GetItemRectSize().y);
		ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImGui::GetColorU32(rect_color));
		ImVec4 button_color = ImGui::GetStyleColorVec4(ImGuiCol_Button);
		button_color.w = 0.0f;
		ImGui::PushStyleColor(ImGuiCol_Button, button_color);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 50);
		if (ImGui::Button(ICON_FA_PLUS))
		{
			LOG_INFO("add component");
		}
		ImGui::SameLine(0, 5);
		if (ImGui::Button(ICON_FA_TRASH))
		{
			LOG_INFO("remove component");
		}
		ImGui::PopStyleColor();

		// add properties
		if (is_tree_open)
		{
			ImGui::PushFont(smallFont());
			ImGui::TableNextRow();
			for (auto& prop : rttr::type::get(*component.get()).get_properties())
			{
				std::string prop_name = prop.get_name().to_string();
				StringUtil::remove(prop_name, "m_");

				EPropertyType property_type = getPropertyType(prop.get_type());
				ASSERT(property_type.second == EPropertyContainerType::Mono, "don't support container property type now");

				rttr::variant& variant = prop.get_value(*component.get());
				m_property_constructors[property_type.first](prop_name, variant);
				prop.set_value(*component.get(), variant);
			}
			ImGui::TreePop();
			ImGui::PopFont();
		}
	}

	void PropertyUI::constructPropertyBool(const std::string& name, rttr::variant& variant)
	{
		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());
		ImGui::PopStyleVar();

		ImGui::TableNextColumn();
		ImGui::Checkbox(label.c_str(), &variant.get_value<bool>());
	}

	void PropertyUI::constructPropertyIntegar(const std::string& name, rttr::variant& variant)
	{
		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::InputInt(label.c_str(), &variant.get_value<int>());
	}

	void PropertyUI::constructPropertyFloat(const std::string& name, rttr::variant& variant)
	{
		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputFloat(label.c_str(), &variant.get_value<float>());
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyString(const std::string& name, rttr::variant& variant)
	{
		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::Text("%s", (*&variant.get_value<std::string>()).c_str());
	}

	void PropertyUI::constructPropertyVec2(const std::string& name, rttr::variant& variant)
	{
		glm::vec2* p_vec2 = &variant.get_value<glm::vec2>();
		float value[2] = { p_vec2->x, p_vec2->y };

		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat2(label.c_str(), value);
		ImGui::PopItemWidth();

		p_vec2->x = value[0];
		p_vec2->y = value[1];
	}

	void PropertyUI::constructPropertyVec3(const std::string& name, rttr::variant& variant)
	{
		glm::vec3* p_vec3 = &variant.get_value<glm::vec3>();
		float value[3] = { p_vec3->x, p_vec3->y, p_vec3->z };

		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat3(label.c_str(), value);
		ImGui::PopItemWidth();

		p_vec3->x = value[0];
		p_vec3->y = value[1];
		p_vec3->z = value[2];
	}

	void PropertyUI::constructPropertyVec4(const std::string& name, rttr::variant& variant)
	{
		glm::vec4* p_vec4 = &variant.get_value<glm::vec4>();
		float value[4] = { p_vec4->x, p_vec4->y, p_vec4->z, p_vec4->w };

		std::string label = "##" + name;
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::DragFloat4(label.c_str(), value);
		ImGui::PopItemWidth();

		p_vec4->x = value[0];
		p_vec4->y = value[1];
		p_vec4->z = value[2];
		p_vec4->w = value[3];
	}

	void PropertyUI::constructPropertyAsset(const std::string& name, rttr::variant& variant)
	{
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("asset: %s", name.c_str());

		ImGui::TableNextColumn();
		ImGui::Text("asset: %s", name.c_str());
	}

	EPropertyType PropertyUI::getPropertyType(const rttr::type& type)
	{
		const std::string& type_name = type.get_name().to_string();
		EPropertyValueType value_type = EPropertyValueType::Bool;
		EPropertyContainerType container_type = EPropertyContainerType::Mono;
		if (type_name.find("std::vector") != std::string::npos)
		{
			container_type = EPropertyContainerType::Array;
		}
		else if (type_name.find("std::map") != std::string::npos)
		{
			container_type = EPropertyContainerType::Map;
		}

		if (type_name.find("glm::vec<2") != std::string::npos)
		{
			value_type = EPropertyValueType::Vec2;
		}
		else if (type_name.find("glm::vec<3") != std::string::npos)
		{
			value_type = EPropertyValueType::Vec3;
		}
		else if (type_name.find("glm::vec<4") != std::string::npos)
		{
			value_type = EPropertyValueType::Vec4;
		}
		else if (type_name.find("std::shared_ptr") != std::string::npos)
		{
			value_type = EPropertyValueType::Asset;
		}
		else if (type_name.find("bool") != std::string::npos)
		{
			value_type = EPropertyValueType::Bool;
		}
		else if (type_name.find("int") != std::string::npos)
		{
			value_type = EPropertyValueType::Integar;
		}
		else if (type_name.find("float") != std::string::npos)
		{
			value_type = EPropertyValueType::Float;
		}
		else if (type_name.find("std::string") != std::string::npos)
		{
			value_type = EPropertyValueType::String;
		}

		return std::make_pair(value_type, container_type);
	}

}