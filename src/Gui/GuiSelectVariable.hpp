#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include "../commons.hpp"
#include "GuiHelper.hpp"
#include "ImguiPlugins.hpp"
#include "Variable.hpp"
#include "VariableHandler.hpp"

class SelectVariableWindow
{
   public:
	SelectVariableWindow(VariableHandler* variableHandler, std::set<std::string>* selection) : variableHandler(variableHandler), selection(selection)
	{
	}

	void draw()
	{
		if (show)
			ImGui::OpenPopup("Select Variables");

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500 * GuiHelper::contentScale, -1), ImGuiCond_Once);

		if (ImGui::BeginPopupModal("Select Variables", &show, 0))
		{
			const char* searchLabel = "search ";
			uint32_t dupa = ImGui::GetItemRectSize().x - 25 * GuiHelper::contentScale;
			uint32_t dupa2 = ImGui::CalcTextSize(searchLabel).x;
			ImGui::PushItemWidth(dupa - dupa2);
			ImGui::Dummy(ImVec2(-1, 5));
			static std::string search{};
			ImGui::Text("%s", searchLabel);
			ImGui::SameLine();
			ImGui::InputText("##search", &search, 0, NULL, NULL);
			ImGui::PopItemWidth();
			ImGui::Dummy(ImVec2(-1, 5));

			drawTable(search);

			std::string importBtnName{"Select ("};
			importBtnName += std::to_string(selection->size()) + std::string(")");

			if (ImGui::Button("Done", ImVec2(-1, 25 * GuiHelper::contentScale)))
			{
				show = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void setShowState(bool state)
	{
		show = state;
	}

   private:
	void drawTable(const std::string& substring)
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

		if (ImGui::BeginTable("table_scrolly", 2, flags, ImVec2(0.0f, 400 * GuiHelper::contentScale)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name", 0);
			ImGui::TableSetupColumn("Address", 0);
			ImGui::TableHeadersRow();

			for (std::shared_ptr<Variable> var : *variableHandler)
			{
				std::string name = var->getName();
				if (toLower(name).find(toLower(substring)) == std::string::npos)
					continue;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				const bool item_is_selected = selection->contains(name);

				ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick;
				if (ImGui::Selectable(name.c_str(), item_is_selected, selectable_flags, ImVec2(0, 12 * GuiHelper::contentScale)))
				{
					if (ImGui::GetIO().KeyCtrl)
					{
						if (item_is_selected)
							selection->erase(name);
						else
							selection->insert(name);
					}
					else
					{
						selection->clear();
						selection->insert(name);
					}

					if (ImGui::IsMouseDoubleClicked(0))
					{
						selection->clear();
						selection->insert(name);
						show = false;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", ("0x" + std::string(GuiHelper::intToHexString(var->getAddress()))).c_str());
			}

			ImGui::EndTable();
		}
	}

   private:
	VariableHandler* variableHandler;

	std::set<std::string>* selection;

	bool show = false;
};