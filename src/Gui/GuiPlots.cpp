#include "Gui.hpp"

std::string dragAndDrop()
{
	std::string name = "";

	if (ImPlot::BeginDragDropTargetPlot())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
			name = *(std::string*)payload->Data;
		ImPlot::EndDragDropTarget();
	}
	return name;
}

void Gui::drawPlots()
{
	uint32_t tablePlots = 0;

	ImVec2 initialCursorPos = ImGui::GetCursorPos();

	for (std::shared_ptr<Plot> plt : *plotHandler)
	{
		if (plt->getType() == Plot::Type::TABLE)
		{
			drawPlotTable(plt.get(), plt->getTimeSeries(), plt->getSeriesMap());
			if (plt->getVisibility())
				tablePlots++;
		}
	}

	uint32_t curveBarPlotsCnt = plotHandler->getVisiblePlotsCount() - tablePlots;
	uint32_t row = curveBarPlotsCnt > 0 ? curveBarPlotsCnt : 1;

	const float remainingSpace = (ImGui::GetWindowPos().y + ImGui::GetWindowSize().y) - (ImGui::GetCursorPos().y + initialCursorPos.y);
	ImVec2 plotSize(-1, -1);
	if (remainingSpace < 300)
		plotSize.y = 300;

	if (ImPlot::BeginSubplots("##subplos", row, 1, plotSize, 0))
	{
		for (std::shared_ptr<Plot> plt : *plotHandler)
		{
			if (!plt->getVisibility())
				continue;

			if (plt->getType() == Plot::Type::CURVE)
				drawPlotCurve(plt.get(), plt->getTimeSeries(), plt->getSeriesMap(), tablePlots);
			else if (plt->getType() == Plot::Type::BAR)
				drawPlotBar(plt.get(), plt->getTimeSeries(), plt->getSeriesMap(), tablePlots);
		}

		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurve(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		if (plotHandler->getViewerState() == PlotHandler::state::RUN)
		{
			PlotHandler::Settings settings = plotHandler->getSettings();
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", 0);
			const double viewportWidth = (settings.samplePeriod > 0 ? settings.samplePeriod : 1) * 0.001f * settings.maxViewportPoints;
			const double min = *time.getLastElement() < viewportWidth ? 0.0f : *time.getLastElement() - viewportWidth;
			const double max = min == 0.0f ? *time.getLastElement() : min + viewportWidth;
			ImPlot::SetupAxisLimits(ImAxis_X1, min, max, ImPlotCond_Always);
		}
		else
		{
			ImPlot::SetupAxes("time[s]", NULL, 0, 0);
			ImPlot::SetupAxisLimits(ImAxis_X1, -1, 10, ImPlotCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 0.1, ImPlotCond_Once);
		}

		plot->setIsHovered(ImPlot::IsPlotHovered());

		if (ImPlot::BeginDragDropTargetPlot())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
				plot->addSeries(*vars[*(std::string*)payload->Data]);

			ImPlot::EndDragDropTarget();
		}

		std::string newSeries = dragAndDrop();
		if (!newSeries.empty())
			plot->addSeries(*vars[newSeries]);

		ImPlotRect plotLimits = ImPlot::GetPlotLimits();

		if (plot->getMarkerStateX0())
		{
			double markerPos = plot->getMarkerValueX0();
			if (markerPos == 0.0)
			{
				markerPos = plotLimits.X.Min + ((std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f);
				plot->setMarkerValueX0(markerPos);
			}
			ImPlot::DragLineX(0, &markerPos, ImVec4(1, 0, 1, 1));
			plot->setMarkerValueX0(markerPos);

			ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(-10, 0), true, "x0 %.5f", markerPos);
		}
		else
			plot->setMarkerValueX0(0.0);

		if (plot->getMarkerStateX1())
		{
			double markerPos = plot->getMarkerValueX1();
			if (markerPos == 0.0)
			{
				markerPos = plotLimits.X.Min + (2.0f * (std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f);
				plot->setMarkerValueX1(markerPos);
			}
			ImPlot::DragLineX(1, &markerPos, ImVec4(1, 1, 0, 1));
			plot->setMarkerValueX1(markerPos);
			ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 0), true, "x1 %.5f", markerPos);
			double dx = markerPos - plot->getMarkerValueX0();
			ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 20), true, "x1-x0 %.5f", dx);
		}
		else
			plot->setMarkerValueX1(0.0);

		/* make thread safe copies of buffers - TODO refactor */
		mtx->lock();
		time.copyData();
		for (auto& [key, serPtr] : seriesMap)
		{
			if (!serPtr->visible)
				continue;
			serPtr->buffer->copyData();
		}
		uint32_t offset = time.getOffset();
		uint32_t size = time.getSize();
		mtx->unlock();

		for (auto& [key, serPtr] : seriesMap)
		{
			if (!serPtr->visible)
				continue;

			const double timepoint = plot->getMarkerValueX0();
			const double value = *(serPtr->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));
			auto name = plot->getMarkerStateX0() ? key + " = " + std::to_string(value) : key;

			ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
			ImPlot::PlotLine(name.c_str(), time.getFirstElementCopy(), serPtr->buffer->getFirstElementCopy(), size, 0, offset, sizeof(double));

			if (plot->getMarkerStateX0())
			{
				ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(255, 255, 255, 255), 0.5f);
				ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
			}
		}

		ImPlot::EndPlot();
	}
}
void Gui::drawPlotBar(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, uint32_t curveBarPlots)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		std::vector<const char*> glabels;
		std::vector<double> positions;

		float pos = 0.0f;
		for (const auto& [key, series] : seriesMap)
		{
			glabels.push_back(series->var->getName().c_str());
			positions.push_back(pos);
			pos += 1.0f;
		}
		glabels.push_back(nullptr);

		ImPlot::SetupAxes(NULL, "Value", 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, -1, seriesMap.size(), ImPlotCond_Always);
		ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), seriesMap.size(), glabels.data());

		std::string newSeries = dragAndDrop();
		if (!newSeries.empty())
			plot->addSeries(*vars[newSeries]);

		double xs = 0.0f;
		double barSize = 0.5f;

		for (auto& [key, serPtr] : seriesMap)
		{
			if (!serPtr->visible)
				continue;
			double value = *serPtr->buffer->getLastElement();

			ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
			ImPlot::PlotBars(serPtr->var->getName().c_str(), &xs, &value, 1, barSize);
			ImPlot::Annotation(xs, value / 2.0f, ImVec4(0, 0, 0, 0), ImVec2(0, -5), true, "%.5f", value);
			xs += 1.0f;
		}
		ImPlot::EndPlot();
	}
}

void Gui::drawPlotTable(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap)
{
	if (!plot->getVisibility())
		return;

	static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(plot->getName().c_str()).x) * 0.5f);
	ImGui::Text(plot->getName().c_str());

	if (ImGui::BeginTable(plot->getName().c_str(), 4, flags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Read value", ImGuiTableColumnFlags_None);
		ImGui::TableSetupColumn("Write value", ImGuiTableColumnFlags_None);
		ImGui::TableHeadersRow();

		for (auto& [key, serPtr] : seriesMap)
		{
			if (!serPtr->visible)
				continue;
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			Variable::Color a = serPtr->var->getColor();
			ImVec4 col = {a.r, a.g, a.b, a.a};
			ImGui::ColorButton("##", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip, ImVec2(10, 10));
			ImGui::SameLine();
			ImGui::Text(key.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text(("0x" + std::string(intToHexString(serPtr->var->getAddress()))).c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::SelectableInput(key.c_str(), false, ImGuiSelectableFlags_None, plot->getSeriesValueString(key, serPtr->var->getValue()).data(), maxVariableNameLength);
			showChangeFormatPopup("format", *plot, key);
			ImGui::TableSetColumnIndex(3);
			ImGui::PushID("input");
			char newValue[maxVariableNameLength] = {0};
			if (ImGui::SelectableInput(key.c_str(), false, ImGuiSelectableFlags_None, newValue, maxVariableNameLength))
			{
				if (plotHandler->getViewerState() == PlotHandler::state::STOP)
				{
					ImGui::PopID();
					continue;
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
				{
					logger->info("New value to be written: {}", newValue);
					if (!plotHandler->writeSeriesValue(*serPtr->var, std::stod(newValue)))
						logger->error("Error while writing new value!");
				}
			}
			ImGui::PopID();
		}
		ImGui::EndTable();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
				plot->addSeries(*vars[*(std::string*)payload->Data]);
			ImGui::EndDragDropTarget();
		}
	}
	plot->setIsHovered(ImGui::IsItemHovered());
}