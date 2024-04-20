#include <future>
#include <string>
#include <unordered_map>
#include <utility>

#include "GdbParser.hpp"
#include "Gui.hpp"
#include "ImguiPlugins.hpp"

void Gui::drawImportVariablesWindow()
{
	static std::unordered_map<std::string, uint32_t> selection;
	static std::future<bool> refreshThread{};

	if (showImportVariablesWindow)
		ImGui::OpenPopup("Import Variables");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500 * contentScale, 500 * contentScale), ImGuiCond_Once);

	if (ImGui::BeginPopupModal("Import Variables", &showImportVariablesWindow, 0))
	{
		char buttonText[30]{};

		if (refreshThread.valid() && refreshThread.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			snprintf(buttonText, 30, "Refresh %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
		else
		{
			if (refreshThread.valid() && !refreshThread.get())
				acqusitionErrorPopup.show("Error!", "Update error. Please check the *.elf file path!", 2.0f);
			snprintf(buttonText, 30, "Refresh");
		}

		if (ImGui::Button(buttonText, ImVec2(-1, 25 * contentScale)))
			refreshThread = std::async(std::launch::async, &GdbParser::parse, parser, projectElfPath);

		static std::string search{};
		ImGui::Text("search ");
		ImGui::SameLine();
		ImGui::InputText("##search", &search, 0, NULL, NULL);
		ImGui::SameLine();
		ImGui::HelpMarker("Import feature is still in Beta. If you're unable to find your variable on the list please open an issue on GitHub (remember to attach your *.elf file).");

		drawImportVariablesTable(parser->getParsedData(), selection, search);

		std::string importBtnName{"Import ("};
		importBtnName += std::to_string(selection.size()) + std::string(")");

		if (ImGui::Button(importBtnName.c_str(), ImVec2(-1, 25 * contentScale)))
		{
			for (auto& [newName, newAddress] : selection)
			{
				addNewVariable(newName);
				vars.at(newName)->setAddress(newAddress);
			}
		}

		if (ImGui::Button("Done", ImVec2(-1, 25 * contentScale)))
		{
			performVariablesUpdate = true;
			showImportVariablesWindow = false;
			ImGui::CloseCurrentPopup();
		}

		acqusitionErrorPopup.handle();

		ImGui::EndPopup();
	}
}

void Gui::drawImportVariablesTable(const std::map<std::string, GdbParser::VariableData>& importedVars, std::unordered_map<std::string, uint32_t>& selection, const std::string& substring)
{
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	if (ImGui::BeginTable("table_scrolly", 2, flags, ImVec2(0.0f, 350 * contentScale)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Name", 0);
		ImGui::TableSetupColumn("Address", 0);
		ImGui::TableHeadersRow();

		for (auto& [name, varData] : importedVars)
		{
			if (toLower(name).find(toLower(substring)) == std::string::npos)
				continue;

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			const bool item_is_selected = selection.contains(name);

			ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
			if (ImGui::Selectable(name.c_str(), item_is_selected, selectable_flags, ImVec2(0, 12 * contentScale)))
			{
				if (ImGui::GetIO().KeyCtrl)
				{
					if (item_is_selected)
						selection.erase(name);
					else
						selection[name] = varData.address;
				}
				else
				{
					selection.clear();
					selection[name] = varData.address;
				}
			}
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", ("0x" + std::string(intToHexString(varData.address))).c_str());
		}

		ImGui::EndTable();
	}
}