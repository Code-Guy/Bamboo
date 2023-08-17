#include "property_ui.h"
#include "runtime/core/event/event_system.h"
#include "runtime/resource/asset/asset_manager.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/core/color/color.h"

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
			{ EPropertyValueType::Color3, std::bind(&PropertyUI::constructPropertyColor3, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Color4, std::bind(&PropertyUI::constructPropertyColor4, this, std::placeholders::_1, std::placeholders::_2) },
			{ EPropertyValueType::Asset, std::bind(&PropertyUI::constructPropertyAsset, this, std::placeholders::_1, std::placeholders::_2) },
		};

		g_runtime_context.eventSystem()->addListener(EventType::UISelectEntity, std::bind(&PropertyUI::onSelectEntity, this, std::placeholders::_1));

		// get dummy texture2d
		std::shared_ptr<Texture2D> m_dummy_texture = g_runtime_context.assetManager()->loadAsset<Texture2D>(DEFAULT_TEXTURE_URL);
		m_dummy_image = loadImGuiImageFromTexture2D(m_dummy_texture);
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

			rttr::type component_type = rttr::type::get(*component.get());
			for (auto& prop : component_type.get_properties())
			{
				std::string prop_name = prop.get_name().to_string();
				EPropertyType property_type = getPropertyType(prop.get_type());
				ASSERT(property_type.second != EPropertyContainerType::Map, "don't support map container property type now");

				rttr::variant& variant = prop.get_value(*component.get());
				if (property_type.second == EPropertyContainerType::Mono)
				{
					m_property_constructors[property_type.first](prop_name, variant);
					prop.set_value(*component.get(), variant);
				}
				else if (property_type.second == EPropertyContainerType::Array)
				{
					auto view = variant.create_sequential_view();
					for (size_t i = 0; i < view.get_size(); ++i)
					{
						rttr::variant sub_variant = view.get_value(i);
						std::string sub_prop_name = prop_name + "_" + std::to_string(i);
						m_property_constructors[property_type.first](sub_prop_name, sub_variant);
						view.set_value(i, sub_variant);
					}
					prop.set_value(*component.get(), variant);
				}
			}
			ImGui::TreePop();
			ImGui::PopFont();
		}
	}

	void PropertyUI::constructPropertyBool(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		std::string label = "##" + name;
		ImGui::Checkbox(label.c_str(), &variant.get_value<bool>());
	}

	void PropertyUI::constructPropertyIntegar(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		std::string label = "##" + name;
		ImGui::InputInt(label.c_str(), &variant.get_value<int>());
	}

	void PropertyUI::constructPropertyFloat(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		ImGui::InputFloat(label.c_str(), &variant.get_value<float>());
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyString(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::Text("%s", (*&variant.get_value<std::string>()).c_str());
	}

	void PropertyUI::constructPropertyVec2(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		glm::vec2& vec2 = variant.get_value<glm::vec2>();
		ImGui::DragFloat2(label.c_str(), glm::value_ptr(vec2));
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyVec3(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		glm::vec3& vec3 = variant.get_value<glm::vec3>();
		ImGui::DragFloat3(label.c_str(), glm::value_ptr(vec3));
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyVec4(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		glm::vec4& vec4 = variant.get_value<glm::vec4>();
		ImGui::DragFloat4(label.c_str(), glm::value_ptr(vec4));
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyColor3(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		Color3& color3 = variant.get_value<Color3>();
		ImGui::ColorEdit3(label.c_str(), color3.data());
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyColor4(const std::string& name, rttr::variant& variant)
	{
		addPropertyNameText(name);

		ImGui::TableNextColumn();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		std::string label = "##" + name;
		Color4& color4 = variant.get_value<Color4>();
		ImGui::ColorEdit3(label.c_str(), color4.data());
		ImGui::PopItemWidth();
	}

	void PropertyUI::constructPropertyAsset(const std::string& name, rttr::variant& variant)
	{
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());

		ImGui::TableNextColumn();

		// asset preview image
		const ImVec2 icon_size(60, 60);
		ImGui::Image(m_dummy_image->tex_id, icon_size);

		// asset find combo box
		ImGui::SameLine();
		const char* asset_names[] = { "asset_0", "asset_1", "asset_2", "asset_3" };
		int selected_index = 0;
		const char* preview_value = asset_names[selected_index];
		ImGuiComboFlags combo_flags = 0;
		if (ImGui::BeginCombo("##select_asset", preview_value, combo_flags))
		{
			for (int i = 0; i < IM_ARRAYSIZE(asset_names); ++i)
			{
				const bool is_selected = selected_index == i;
				if (ImGui::Selectable(asset_names[i], is_selected))
				{
					selected_index = i;
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}	
			}
			ImGui::EndCombo();
		}
	}

	void PropertyUI::addPropertyNameText(const std::string& name)
	{
		ImGui::TableNextColumn();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::Text("%s", name.c_str());
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
		else if (type_name.find("Color3") != std::string::npos)
		{
			value_type = EPropertyValueType::Color3;
		}
		else if (type_name.find("Color4") != std::string::npos)
		{
			value_type = EPropertyValueType::Color4;
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