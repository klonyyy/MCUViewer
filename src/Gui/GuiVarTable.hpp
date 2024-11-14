#pragma once

#include <set>
#include <string>

#include "GdbParser.hpp"
#include "GuiHelper.hpp"
#include "GuiImportVariables.hpp"
#include "GuiVariablesEdit.hpp"
#include "PlotHandlerBase.hpp"
#include "Popup.hpp"
#include "Variable.hpp"
#include "VariableHandler.hpp"

class VariableTableWindow
{
   public:
	VariableTableWindow(PlotHandler* plotHandler, VariableHandler* variableHandler, std::string* projectElfPath, std::string* projectConfigPath, spdlog::logger* logger) : plotHandler(plotHandler), variableHandler(variableHandler), projectElfPath(projectElfPath), projectConfigPath(projectConfigPath), logger(logger)
	{
		parser = std::make_shared<GdbParser>(variableHandler, logger);
		variableEditWindow = std::make_shared<VariableEditWindow>(variableHandler);
		importVariablesWindow = std::make_shared<ImportVariablesWindow>(parser.get(), projectElfPath, projectConfigPath, variableHandler);
	}

	void draw()
	{
		static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;
		static std::set<std::string> selection;

		ImGui::BeginDisabled(plotHandler->getViewerState() == PlotHandlerBase::state::RUN);
		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Variables");
		ImGui::SameLine();
		ImGui::HelpMarker("Select your *.elf file in the Options->Acqusition Settings to import or update the variables.");
		ImGui::Separator();

		drawAddVariableButton();
		drawUpdateAddressesFromElf();

		const char* label = "search ";
		ImGui::PushItemWidth(ImGui::GetItemRectSize().x - ImGui::CalcTextSize(label).x - 8 * GuiHelper::contentScale);
		static std::string search{};
		ImGui::Text("%s", label);
		ImGui::SameLine();
		ImGui::InputText("##search", &search, 0, NULL, NULL);
		ImGui::PopItemWidth();

		if (ImGui::BeginTable("table_scrolly", 3, flags, ImVec2(0.0f, 300 * GuiHelper::contentScale)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Name", 0);
			ImGui::TableSetupColumn("Address", 0);
			ImGui::TableSetupColumn("Type", 0);
			ImGui::TableHeadersRow();

			std::optional<std::string> varNameToDelete;
			std::string currentName{};

			for (std::shared_ptr<Variable> var : *variableHandler)
			{
				std::string name = var->getName();
				if (toLower(name).find(toLower(search)) == std::string::npos)
					continue;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::PushID(name.c_str());
				ImGui::ColorEdit4("##", &var->getColor().r, ImGuiColorEditFlags_NoInputs);
				ImGui::SameLine();
				ImGui::PopID();
				char variable[Variable::maxVariableNameLength] = {0};
				std::memcpy(variable, var->getName().data(), var->getName().length());

				const bool itemIsSelected = selection.contains(name);

				if (ImGui::Selectable(var->getName().c_str(), itemIsSelected, ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (ImGui::IsMouseDoubleClicked(0))
					{
						variableEditWindow->setVariableToEdit(var);
						variableEditWindow->setShowVariableEditWindowState(true);
					}

					if (ImGui::GetIO().KeyCtrl && var->getIsFound())
					{
						if (itemIsSelected)
							selection.erase(name);
						else
							selection.insert(name);
					}
					else
						selection.clear();
				}

				drawMenuVariablePopup(name, [&]()
									  { variableHandler->addNewVariable(""); }, [&](std::string name)
									  { variableHandler->addNewVariable(name); }, [&](std::string name)
									  { varNameToDelete = name; }, [&](std::string name)
									  {	variableEditWindow->setVariableToEdit(var);
                                                                    variableEditWindow->setShowVariableEditWindowState(true); });

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					if (selection.empty())
						selection.insert(name);

					/* pass a pointer to the selection as we have to call clear() on the original object upon receiving */
					std::set<std::string>* selectionPtr = &selection;
					ImGui::SetDragDropPayload("MY_DND", &selectionPtr, sizeof(selectionPtr));
					ImGui::PushID(name.c_str());
					ImGui::ColorEdit4("##", &variableHandler->getVariable(*selection.begin())->getColor().r, ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::PopID();

					if (selection.size() > 1)
						ImGui::TextUnformatted("<multiple vars>");
					else
						ImGui::TextUnformatted(selection.begin()->c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::TableSetColumnIndex(1);
				if (var->getIsFound())
					ImGui::Text("%s", ("0x" + std::string(GuiHelper::intToHexString(var->getAddress()))).c_str());
				else
					ImGui::Text("NOT FOUND!");
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%s", var->getTypeStr().c_str());
			}

			if (varNameToDelete.has_value())
			{
				for (std::shared_ptr<Plot> plt : *plotHandler)
					plt->removeSeries(varNameToDelete.value_or(""));
				variableHandler->erase(varNameToDelete.value_or(""));
			}
			ImGui::EndTable();
		}

		ImGui::EndDisabled();

		variableEditWindow->draw();
		importVariablesWindow->draw();
	}

   private:
	void drawAddVariableButton()
	{
		if (ImGui::Button("Add variable", ImVec2(-1, 25 * GuiHelper::contentScale)))
		{
			variableHandler->addNewVariable("");
		}

		ImGui::BeginDisabled(projectElfPath->empty());

		if (ImGui::Button("Import variables from *.elf", ImVec2(-1, 25 * GuiHelper::contentScale)))
			importVariablesWindow->setShowImportVariablesWindow(true);

		ImGui::EndDisabled();
	}

	void drawUpdateAddressesFromElf()
	{
		static std::future<bool> refreshThread{};
		static bool shouldPopStyle = false;

		static constexpr size_t textSize = 40;
		char buttonText[textSize]{};

		if (refreshThread.valid() && refreshThread.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			snprintf(buttonText, textSize, "Update variable addresses %c", "|/-\\"[(int)(ImGui::GetTime() / 0.05f) & 3]);
		else
		{
			snprintf(buttonText, textSize, "Update variable addresses");
			if (refreshThread.valid() && !refreshThread.get())
				popup.show("Error!", "Update error. Please check the *.elf file path!", 2.0f);
		}

		ImGui::BeginDisabled(projectElfPath->empty());

		bool elfChanged = checkElfFileChanged();

		performVariablesUpdate = importVariablesWindow->shouldPerformVariableUpdate();

		if (elfChanged)
		{
			ImVec4 color = ImColor::HSV(0.1f, 0.97f, 0.72f);
			snprintf(buttonText, textSize, "Click to reload *.elf changes!");
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

			if (plotHandler->getSettings().stopAcqusitionOnElfChange)
				plotHandler->setViewerState(PlotHandlerBase::state::STOP);

			if (plotHandler->getSettings().refreshAddressesOnElfChange)
			{
				performVariablesUpdate = true;
				/* TODO: examine why elf is not ready to be parsed without the delay */
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			shouldPopStyle = true;
		}

		if (ImGui::Button(buttonText, ImVec2(-1, 25 * GuiHelper::contentScale)) || performVariablesUpdate)
		{
			parser->changeCurrentGDBCommand(plotHandler->getSettings().gdbCommand);
			lastModifiedTime = std::filesystem::file_time_type::clock::now();
			refreshThread = std::async(std::launch::async, &GdbParser::updateVariableMap, parser, GuiHelper::convertProjectPathToAbsolute(projectElfPath, projectConfigPath));
			performVariablesUpdate = false;
		}

		/* TODO fix this ugly solution */
		if (shouldPopStyle)
		{
			ImGui::PopStyleColor(3);
			shouldPopStyle = false;
		}
		ImGui::EndDisabled();
	}

	bool checkElfFileChanged()
	{
		std::string path = GuiHelper::convertProjectPathToAbsolute(projectElfPath, projectConfigPath);
		if (!std::filesystem::exists(path))
			return false;

		auto writeTime = std::filesystem::last_write_time(path);
		return writeTime > lastModifiedTime;
	}

	void drawMenuVariablePopup(const std::string& name, std::function<void()> onNew, std::function<void(const std::string&)> onCopy, std::function<void(const std::string&)> onDelete, std::function<void(const std::string&)> onProperties)
	{
		ImGui::PushID(name.c_str());
		if (ImGui::BeginPopupContextItem(name.c_str()))
		{
			if (ImGui::MenuItem("New"))
				onNew();

			if (ImGui::MenuItem("Copy"))
				onCopy(name);

			if (ImGui::MenuItem("Delete"))
				onDelete(name);

			if (ImGui::MenuItem("Properties"))
				onProperties(name);

			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

   private:
	PlotHandler* plotHandler;
	VariableHandler* variableHandler;
	std::string* projectElfPath;
	std::string* projectConfigPath;
	spdlog::logger* logger;

	Popup popup;
	std::shared_ptr<GdbParser> parser;
	std::shared_ptr<VariableEditWindow> variableEditWindow;
	std::shared_ptr<ImportVariablesWindow> importVariablesWindow;

	std::filesystem::file_time_type lastModifiedTime = std::filesystem::file_time_type::clock::now();

	bool performVariablesUpdate = false;
};