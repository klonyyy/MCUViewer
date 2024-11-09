#pragma once

#include <optional>
#include <string>

#include "GuiHelper.hpp"
#include "GuiPlotEdit.hpp"
#include "GuiStatisticsWindow.hpp"
#include "IFileHandler.hpp"
#include "Plot.hpp"
#include "PlotGroupHandler.hpp"
#include "PlotHandler.hpp"

class PlotsTree
{
   public:
	PlotsTree(PlotHandler* plotHandler, PlotGroupHandler* plotGroupHandler, std::map<std::string, std::shared_ptr<Variable>>& vars, std::shared_ptr<PlotEditWindow> plotEditWindow, IFileHandler* fileHandler, spdlog::logger* logger) : plotHandler(plotHandler), plotGroupHandler(plotGroupHandler), vars(vars), plotEditWindow(plotEditWindow), fileHandler(fileHandler), logger(logger)
	{
	}
	void draw()
	{
		const uint32_t windowHeight = 320 * GuiHelper::contentScale;
		static std::string selectedGroup = "";
		static std::string selectedPlot = "";
		std::optional<std::string> plotNameToDelete = {};

		ImGui::Dummy(ImVec2(-1, 5));
		GuiHelper::drawCenteredText("Plots");
		ImGui::Separator();

		drawAddPlotButton();

		if (plotHandler->getPlotsCount() == 0)
		{
			selectedGroup = "new group0";
			selectedPlot = "new plot0";
			auto group = plotGroupHandler->addGroup("new group0");
			auto plot = plotHandler->addPlot("new plot0");
			group->addPlot(plot);
		}

		if (!plotHandler->checkIfPlotExists(selectedPlot))
			selectedPlot = plotHandler->begin().operator*()->getName();

		ImGui::BeginChild("Plot Tree", ImVec2(-1, windowHeight));
		ImGui::BeginChild("left pane", ImVec2(150 * GuiHelper::contentScale, -1), true);

		ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
		std::optional<std::string> groupNameToDelete;

		for (auto& [name, group] : *plotGroupHandler)
		{
			ImGuiTreeNodeFlags node_flags = base_flags;
			if (selectedGroup == name)
			{
				node_flags |= ImGuiTreeNodeFlags_Selected;
				plotGroupHandler->setActiveGroup(name);
			}

			bool state = ImGui::TreeNodeEx(group->getName().c_str(), node_flags);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
				selectedGroup = name;

			if (!groupNameToDelete.has_value())
				groupNameToDelete = showMenuGroupPopup("delete", name);

			if (state)
			{
				/* Drag n Drop target for plots within groups */
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PLOT"))
					{
						std::string selection = *(std::string*)payload->Data;
						group->addPlot(plotHandler->getPlot(selection));
					}
					ImGui::EndDragDropTarget();
				}

				for (auto& [name, plot] : *group)
				{
					ImGui::Checkbox(std::string("##" + name).c_str(), &plot->getVisibilityVar());
					ImGui::SameLine();

					if (ImGui::Selectable(name.c_str(), selectedPlot == name, ImGuiSelectableFlags_AllowDoubleClick))
					{
						selectedPlot = name;

						if (ImGui::IsMouseDoubleClicked(0))
						{
							plotEditWindow->setPlotToEdit(plot);
							plotEditWindow->setShowPlotEditWindowState(true);
						}
					}

					if (!plotNameToDelete.has_value())
						plotNameToDelete = GuiHelper::showDeletePopup("Delete", name);

					/* Drag n Drop source for plots within groups */
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						ImGui::SetDragDropPayload("PLOT", &name, sizeof(name));
						ImGui::TextUnformatted(name.c_str());
						ImGui::EndDragDropSource();
					}

					if (plot->isHovered() && ImGui::IsMouseClicked(0))
						selectedPlot = plot->getName();
				}
				ImGui::TreePop();
			}

			if (plotNameToDelete.has_value())
				group->removePlot(plotNameToDelete.value_or(""));
		}

		if (groupNameToDelete.has_value())
		{
			plotGroupHandler->removeGroup(groupNameToDelete.value());
		}

		ImGui::EndChild();
		ImGui::SameLine();

		std::shared_ptr<Plot> plt = plotHandler->getPlot(selectedPlot);
		ImGui::BeginGroup();
		ImGui::PushID(plt->getName().c_str());

		/* Staticstics */
		if (plt->getType() == Plot::Type::CURVE)
		{
			bool mx0 = plt->markerX0.getState();
			bool mx1 = plt->markerX1.getState();
			ImGui::Text("x0 marker  ");
			ImGui::SameLine();
			ImGui::Checkbox("##mx0", &mx0);
			plt->markerX0.setState(mx0);
			ImGui::Text("x1 marker  ");
			ImGui::SameLine();
			ImGui::Checkbox("##mx1", &mx1);
			plt->markerX1.setState(mx1);
			statisticsWindow.drawAnalog(plt);
		}
		ImGui::PopID();

		/* Var list within plot*/
		ImGui::PushID("list");
		if (ImGui::BeginListBox("##", ImVec2(-1, 175 * GuiHelper::contentScale)))
		{
			std::optional<std::string> seriesNameToDelete = {};
			for (auto& [name, ser] : plt->getSeriesMap())
			{
				ImGui::PushID(name.c_str());
				ImGui::Checkbox("", &ser->visible);
				ImGui::PopID();
				ImGui::SameLine();
				ImGui::PushID(name.c_str());
				ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
				ImGui::SameLine();
				ImGui::Selectable(name.c_str());
				if (!seriesNameToDelete.has_value())
					seriesNameToDelete = GuiHelper::showDeletePopup("Delete var", name);
				ImGui::PopID();
			}
			plt->removeSeries(seriesNameToDelete.value_or(""));

			ImGui::EndListBox();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
			{
				std::set<std::string>* selection = *(std::set<std::string>**)payload->Data;

				for (const auto& name : *selection)
					plt->addSeries(*vars[name]);
				selection->clear();
			}
			ImGui::EndDragDropTarget();
		}
		drawExportPlotToCSVButton(plt);
		ImGui::PopID();
		ImGui::EndGroup();
		ImGui::EndChild();
	}

	void drawAddPlotButton()
	{
		if (ImGui::Button("Add plot", ImVec2(-1, 25 * GuiHelper::contentScale)))
		{
			uint32_t num = 0;
			while (plotHandler->checkIfPlotExists(std::string("new plot") + std::to_string(num)))
				num++;

			std::string newName = std::string("new plot") + std::to_string(num);
			auto plot = plotHandler->addPlot(newName);
			plotGroupHandler->getActiveGroup()->addPlot(plot);
		}

		if (ImGui::Button("Add group", ImVec2(-1, 25 * GuiHelper::contentScale)))
		{
			uint32_t num = 0;
			while (plotGroupHandler->checkIfGroupExists(std::string("new group") + std::to_string(num)))
				num++;

			std::string newName = std::string("new group") + std::to_string(num);
			plotGroupHandler->addGroup(newName);
		}
	}

	void drawExportPlotToCSVButton(std::shared_ptr<Plot> plt)
	{
		if (ImGui::Button("Export plot to *.csv", ImVec2(-1, 25 * GuiHelper::contentScale)))
		{
			std::string path = fileHandler->saveFile(std::pair<std::string, std::string>("CSV", "csv"));
			std::ofstream csvFile(path);

			if (!csvFile)
			{
				logger->info("Error opening the file: {}", path);
				return;
			}

			uint32_t dataSize = plt->getXAxisSeries()->getSize();

			csvFile << "time [s],";

			for (auto& [name, ser] : plt->getSeriesMap())
				csvFile << name << ",";

			csvFile << std::endl;

			for (size_t i = 0; i < dataSize; ++i)
			{
				uint32_t offset = plt->getXAxisSeries()->getOffset();
				uint32_t index = (offset + i < dataSize) ? offset + i : i - (dataSize - offset);
				csvFile << plt->getXAxisSeries()->getFirstElementCopy()[index] << ",";

				for (auto& [name, ser] : plt->getSeriesMap())
					csvFile << ser->buffer->getFirstElementCopy()[index] << ",";

				csvFile << std::endl;
			}

			csvFile.close();
		}
	}

   private:
	PlotHandler* plotHandler;
	PlotGroupHandler* plotGroupHandler;
	std::map<std::string, std::shared_ptr<Variable>>& vars;
    	std::shared_ptr<PlotEditWindow> plotEditWindow;

	StatisticsWindow statisticsWindow;
	IFileHandler* fileHandler;
    	spdlog::logger* logger;

	std::optional<std::string>
	showMenuGroupPopup(const char* text, const std::string& name)
	{
		ImGui::PushID(name.c_str());
		if (ImGui::BeginPopupContextItem(text))
		{
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Group"))
				{
				}
				if (ImGui::MenuItem("Plot"))
				{
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Rename"))
			{
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return std::nullopt;
	}
};