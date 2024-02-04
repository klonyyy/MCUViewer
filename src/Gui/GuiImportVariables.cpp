#include <string>
#include <unordered_map>
#include <utility>

#include "GdbParser.hpp"
#include "Gui.hpp"
#include "ImguiPlugins.hpp"

void Gui::drawImportVariablesWindow()
{
	static std::unordered_map<std::string, uint32_t> selection;

	if (showImportVariablesWindow)
		ImGui::OpenPopup("Import Variables");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_Once);

	if (ImGui::BeginPopupModal("Import Variables", &showImportVariablesWindow, 0))
	{
		if (ImGui::Button("Refresh", ImVec2(-1, 20)))
		{
			parser->parse(projectElfPath);
		}

		static std::string search{};
		ImGui::Text("search ");
		ImGui::SameLine();
		ImGui::InputText("##search", &search, 0, NULL, NULL);

		drawImportVariablesTable(parser->getParsedData(), selection, search);

		std::string importBtnName{"Import ("};
		importBtnName += std::to_string(selection.size()) + std::string(")");

		if (ImGui::Button(importBtnName.c_str(), ImVec2(-1, 20)))
		{
			for (auto& [newName, newAddress] : selection)
			{
				addNewVariable(newName);
				vars.at(newName)->setAddress(newAddress);
			}
		}

		if (ImGui::Button("Done", ImVec2(-1, 20)))
		{
			showImportVariablesWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Gui::drawImportVariablesTable(const std::vector<GdbParser::VariableData>& importedVars, std::unordered_map<std::string, uint32_t>& selection, const std::string& substring)
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	if (ImGui::BeginTable("table_scrolly", 2, flags, ImVec2(0.0f, 350)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", 0);
		ImGui::TableSetupColumn("Address", 0);
		ImGui::TableHeadersRow();

		for (auto& [name, address, trivial] : importedVars)
		{
			if (name.find(substring) == std::string::npos)
				continue;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			const bool item_is_selected = selection.contains(name);

			ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
			if (ImGui::Selectable(name.c_str(), item_is_selected, selectable_flags, ImVec2(0, 12)))
			{
				if (ImGui::GetIO().KeyCtrl)
				{
					if (item_is_selected)
						selection.erase(name);
					else
						selection[name] = address;
				}
				else
				{
					selection.clear();
					selection[name] = address;
				}
			}
			ImGui::TableSetColumnIndex(1);
			ImGui::Text(("0x" + std::string(intToHexString(address))).c_str());
		}

		ImGui::EndTable();
	}
}