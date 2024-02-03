#include <unordered_set>

#include "GdbParser.hpp"
#include "Gui.hpp"

void Gui::drawImportVariablesWindow()
{
	static std::unordered_set<std::string> selection;

	if (showImportVariablesWindow)
		ImGui::OpenPopup("Import Variables");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 400));
	if (ImGui::BeginPopupModal("Import Variables", &showImportVariablesWindow, 0))
	{
		if (ImGui::Button("Refresh", ImVec2(-1, 20)))
		{
			parser->parse(projectElfPath);
		}

		drawImportVariablesTable(parser->getParsedData(), selection);

		if (ImGui::Button("Import", ImVec2(-1, 20)))
		{
			for (auto& newName : selection)
				addNewVariable(newName);
		}

		if (ImGui::Button("Done", ImVec2(-1, 20)))
		{
			showImportVariablesWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Gui::drawImportVariablesTable(const std::vector<GdbParser::VariableData>& importedVars, std::unordered_set<std::string>& selection)
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	if (ImGui::BeginTable("table_scrolly", 1, flags, ImVec2(0.0f, 300)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", 0);
		ImGui::TableSetupColumn("Type", 0);
		ImGui::TableHeadersRow();

		for (auto& [name, filepath, trivial] : importedVars)
		{
			ImGui::TableSetColumnIndex(0);
			const bool item_is_selected = selection.contains(name);

			ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
			if (ImGui::Selectable(name.c_str(), item_is_selected, selectable_flags, ImVec2(0, 20)))
			{
				if (ImGui::GetIO().KeyCtrl)
				{
					std::cout << name << std::endl;

					if (item_is_selected)
						selection.erase(name);
					else
						selection.insert(name);
				}
				else
				{
					std::cout << name << std::endl;
					selection.clear();
					selection.insert(name);
				}
			}
			ImGui::TableNextRow();
		}

		ImGui::EndTable();
	}
}