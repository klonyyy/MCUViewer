#include <future>
#include <string>
#include <unordered_map>
#include <utility>

#include "GdbParser.hpp"
#include "Gui.hpp"
#include "ImguiPlugins.hpp"
#include "Popup.hpp"
#include "VariableHandler.hpp"

class ImportVariablesWindow
{
   public:
	ImportVariablesWindow(GdbParser* parser, std::string* projectElfPath, std::string* projectConfigPath, VariableHandler* variableHandler) : parser(parser), projectElfPath(projectElfPath), projectConfigPath(projectConfigPath), variableHandler(variableHandler)
	{
	}

	void draw()
	{
		static std::unordered_map<std::string, uint32_t> selection;
		static std::future<bool> refreshThread{};
		static bool wasPreviouslyOpened = false;
		static bool shouldUpdateOnOpen = false;

		if (showImportVariablesWindow)
		{
			ImGui::OpenPopup("Import Variables");

			if (!wasPreviouslyOpened)
			{
				selection.clear();
				shouldUpdateOnOpen = true;
			}
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500 * GuiHelper::contentScale, 500 * GuiHelper::contentScale), ImGuiCond_Once);

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

			if (ImGui::Button(buttonText, ImVec2(-1, 25 * GuiHelper::contentScale)) || shouldUpdateOnOpen)
			{
				refreshThread = std::async(std::launch::async, &GdbParser::parse, parser, GuiHelper::convertProjectPathToAbsolute(projectElfPath, projectConfigPath));
				shouldUpdateOnOpen = false;
			}

			static std::string search{};
			ImGui::Text("search ");
			ImGui::SameLine();
			ImGui::InputText("##search", &search, 0, NULL, NULL);
			ImGui::SameLine();
			ImGui::HelpMarker("Import feature is still in Beta. If you're unable to find your variable on the list please open an issue on GitHub (remember to attach your *.elf file).");

			drawImportVariablesTable(parser->getParsedData(), selection, search);

			std::string importBtnName{"Import ("};
			importBtnName += std::to_string(selection.size()) + std::string(")");

			if (ImGui::Button(importBtnName.c_str(), ImVec2(-1, 25 * GuiHelper::contentScale)))
			{
				std::vector<std::string> namesAlreadyImported;
				for (auto& [newName, newAddress] : selection)
				{
					if (!variableHandler->contains(newName))
					{
						variableHandler->addNewVariable(newName);
						variableHandler->getVariable(newName)->setAddress(newAddress);
					}
					else
						namesAlreadyImported.push_back(newName);
				}

				if (namesAlreadyImported.size() > 0)
				{
					std::string message = "The following variables were already imported: \n";

					for (auto& name : namesAlreadyImported)
					{
						message += "-" + name;
						if (name != namesAlreadyImported.back())
							message += ", ";
						message += "\n";
					}

					importWarningPopup.show("Warning!", message.c_str(), 2.0f);
				}
				shouldUpdate = true;
			}

			if (ImGui::Button("Done", ImVec2(-1, 25 * GuiHelper::contentScale)))
			{
				showImportVariablesWindow = false;
				ImGui::CloseCurrentPopup();
			}

			acqusitionErrorPopup.handle();
			importWarningPopup.handle();

			ImGui::EndPopup();
		}

		wasPreviouslyOpened = showImportVariablesWindow;
	}

	void setShowImportVariablesWindow(bool show)
	{
		showImportVariablesWindow = show;
	}

	bool shouldPerformVariableUpdate()
	{
		bool temp = shouldUpdate;
		shouldUpdate = false;
		return temp;
	}

   private:
	void drawImportVariablesTable(const std::map<std::string, GdbParser::VariableData>& importedVars, std::unordered_map<std::string, uint32_t>& selection, const std::string& substring)
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

		if (ImGui::BeginTable("table_scrolly", 2, flags, ImVec2(0.0f, 350 * GuiHelper::contentScale)))
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
				if (ImGui::Selectable(name.c_str(), item_is_selected, selectable_flags, ImVec2(0, 12 * GuiHelper::contentScale)))
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
				ImGui::Text("%s", ("0x" + std::string(GuiHelper::intToHexString(varData.address))).c_str());
			}

			ImGui::EndTable();
		}
	}

   private:
	GdbParser* parser;
	std::string* projectElfPath;
	std::string* projectConfigPath;

	VariableHandler* variableHandler;

	Popup acqusitionErrorPopup;
	Popup importWarningPopup;

	bool showImportVariablesWindow = false;
	bool shouldUpdate = false;
};