#include "Gui.hpp"

void Gui::dragAndDropPlot(std::shared_ptr<Plot> plot)
{
	if (ImPlot::BeginDragDropTargetPlot())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
		{
			std::set<std::string>* selection = *(std::set<std::string>**)payload->Data;

			for (const auto& name : *selection)
				plot->addSeries(variableHandler->getVariable(name).get());
			selection->clear();
		}
		ImPlot::EndDragDropTarget();
	}
}

void Gui::drawPlots()
{
	uint32_t tablePlots = 0;
	ImVec2 initialCursorPos = ImGui::GetCursorPos();
	auto activeGroup = plotGroupHandler->getActiveGroup();

	for (auto [name, plotElem] : *activeGroup)
	{
		auto plot = plotElem.plot;

		if (plot->getType() == Plot::Type::TABLE)
		{
			if (plotElem.visibility)
			{
				tablePlots++;
				drawPlotTable(plot);
			}
		}
	}

	uint32_t curveBarPlotsCnt = activeGroup->getVisiblePlotsCount() - tablePlots;
	uint32_t row = curveBarPlotsCnt > 0 ? curveBarPlotsCnt : 1;

	const float remainingSpace = (ImGui::GetWindowPos().y + ImGui::GetWindowSize().y) - (ImGui::GetCursorPos().y + initialCursorPos.y);
	ImVec2 plotSize(-1, -1);
	if (remainingSpace < 300)
		plotSize.y = 300;

	if (ImPlot::BeginSubplots("##subplos", row, 1, plotSize, 0))
	{
		auto activeGroup = plotGroupHandler->getActiveGroup();

		for (auto [name, plotElem] : *activeGroup)
		{
			auto plot = plotElem.plot;

			if (!plotElem.visibility)
				continue;

			if (plot->getType() == Plot::Type::CURVE)
				drawPlotCurve(plot);
			else if (plot->getType() == Plot::Type::BAR)
				drawPlotBar(plot);
			else if (plot->getType() == Plot::Type::XY)
				drawPlotXY(plot);
		}

		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotXY(std::shared_ptr<Plot> plot)
{
	auto& time = *plot->getXAxisSeries();
	auto& seriesMap = plot->getSeriesMap();

	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		Variable* xAxisVariable = plot->getXAxisVariable();
		std::string xLabel = xAxisVariable ? xAxisVariable->getName() : "";

		if (viewerDataHandler->getState() == DataHandlerBase::State::RUN)
		{
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxis(ImAxis_X1, xLabel.c_str(), ImPlotAxisFlags_AutoFit);
		}
		else
		{
			ImPlot::SetupAxes(xLabel.c_str(), NULL, 0, 0);
			ImPlot::SetupAxisLimits(ImAxis_X1, -1, 10, ImPlotCond_Once);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1, 0.1, ImPlotCond_Once);
		}

		plot->setIsHovered(ImPlot::IsPlotHovered());
		dragAndDropPlot(plot);

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

			ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
			ImPlot::PlotLine(key.c_str(), time.getFirstElementCopy(), serPtr->buffer->getFirstElementCopy(), size, 0, offset, sizeof(double));
		}

		ImPlot::EndPlot();
	}
}

void Gui::drawPlotCurve(std::shared_ptr<Plot> plot)
{
	auto& time = *plot->getXAxisSeries();
	auto& seriesMap = plot->getSeriesMap();

	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		if (viewerDataHandler->getState() == DataHandlerBase::State::RUN)
		{
			ViewerDataHandler::Settings settings = viewerDataHandler->getSettings();
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", 0);
			const double viewportWidth = (1.0 / viewerDataHandler->getAverageSamplingFrequency()) * settings.maxViewportPoints;
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
		dragAndDropPlot(plot);

		if (viewerDataHandler->getState() == DataHandlerBase::State::STOP)
		{
			ImPlotRect plotLimits = ImPlot::GetPlotLimits();
			handleMarkers(0, plot->markerX0, plotLimits, [&]()
						  { ImPlot::Annotation(plot->markerX0.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(-10 * GuiHelper::contentScale, 0), true, "x0 %.5f", plot->markerX0.getValue()); });

			handleMarkers(1, plot->markerX1, plotLimits, [&]()
						  {
			ImPlot::Annotation(plot->markerX1.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10*GuiHelper::contentScale, 0), true, "x1 %.5f", plot->markerX1.getValue());
			double dx = plot->markerX1.getValue() - plot->markerX0.getValue();
			ImPlot::Annotation(plot->markerX1.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10*GuiHelper::contentScale, 20*GuiHelper::contentScale), true, "x1-x0 %.5f", dx); });

			handleDragRect(0, plot->stats, plotLimits);
		}

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

			const double timepoint = plot->markerX0.getValue();
			const double value = *(serPtr->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));
			auto name = plot->markerX0.getState() ? key + " = " + std::to_string(value) : key;

			ImPlot::SetNextLineStyle(ImVec4(serPtr->var->getColor().r, serPtr->var->getColor().g, serPtr->var->getColor().b, 1.0f));
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
			ImPlot::PlotLine(name.c_str(), time.getFirstElementCopy(), serPtr->buffer->getFirstElementCopy(), size, ImPlotLineFlags_None, offset, sizeof(double));

			if (plot->markerX0.getState())
			{
				ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(255, 255, 255, 255), 0.5f);
				ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
			}
		}

		ImPlot::EndPlot();
	}
}
void Gui::drawPlotBar(std::shared_ptr<Plot> plot)
{
	auto& seriesMap = plot->getSeriesMap();

	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		std::vector<const char*> glabels;
		std::vector<double> positions;

		float pos = 0.0f;
		float visiblePlotsCnt = 0;
		for (const auto& [name, series] : seriesMap)
		{
			if (!series->visible)
				continue;

			glabels.push_back(name.c_str());
			positions.push_back(pos);
			pos += 1.0f;
			visiblePlotsCnt++;
		}
		glabels.push_back(nullptr);

		ImPlot::SetupAxes(NULL, "Value", 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, -1, visiblePlotsCnt, ImPlotCond_Always);
		ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), visiblePlotsCnt, glabels.data());

		dragAndDropPlot(plot);

		double xs = 0.0f;
		double barSize = 0.5f;

		for (auto& [name, series] : seriesMap)
		{
			if (!series->visible)
				continue;
			double value = *series->buffer->getLastElement();

			ImPlot::SetNextLineStyle(ImVec4(series->var->getColor().r, series->var->getColor().g, series->var->getColor().b, 1.0f));
			ImPlot::SetNextFillStyle(ImVec4(series->var->getColor().r, series->var->getColor().g, series->var->getColor().b, 1.0f));
			ImPlot::PlotBars(name.c_str(), &xs, &value, 1, barSize);
			ImPlot::Annotation(xs, value / 2.0f, ImVec4(0, 0, 0, 0), ImVec2(0, -5 * GuiHelper::contentScale), true, "%.5f", value);
			xs += 1.0f;
		}
		ImPlot::EndPlot();
	}
}

void Gui::drawPlotTable(std::shared_ptr<Plot> plot)
{
	auto& seriesMap = plot->getSeriesMap();

	static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable;

	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(plot->getName().c_str()).x) * 0.5f);
	ImGui::Text("%s", plot->getName().c_str());

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

			ImGui::BeginDisabled(!serPtr->var->getIsCurrentlySampled() && viewerDataHandler->getState() == DataHandlerBase::State::RUN);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			Variable::Color color = serPtr->var->getColor();
			ImVec4 col = {color.r, color.g, color.b, color.a};
			ImGui::ColorButton("##", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip, ImVec2(10 * GuiHelper::contentScale, 10 * GuiHelper::contentScale));
			ImGui::SameLine();
			ImGui::Text("%s", key.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", ("0x" + std::string(GuiHelper::intToHexString(serPtr->var->getAddress()))).c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%s", plot->getSeriesValueString(key, serPtr->var->getValue()).data());
			showChangeFormatPopup("format", *plot, key);
			ImGui::TableSetColumnIndex(3);

			std::string valueToWrite = "";
			std::string inputName = "##Input" + key;

			ImVec4 cellBgColor = ImGui::GetStyle().Colors[ImGuiCol_TableRowBg];
			ImGui::PushStyleColor(ImGuiCol_FrameBg, cellBgColor);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, cellBgColor);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, cellBgColor);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

			if (ImGui::InputText(inputName.c_str(), &valueToWrite, ImGuiInputTextFlags_CharsDecimal, NULL, NULL))
			{
				if (viewerDataHandler->getState() == DataHandlerBase::State::STOP)
				{
					ImGui::PopStyleColor(3);
					continue;
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
				{
					logger->info("New value to be written: {}", valueToWrite);
					if (!viewerDataHandler->writeSeriesValue(*serPtr->var, std::stod(valueToWrite)))
						logger->error("Error while writing new value!");
				}
			}
			ImGui::PopStyleColor(3);
			ImGui::EndDisabled();
		}
		ImGui::EndTable();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MY_DND"))
			{
				std::set<std::string>* selection = *(std::set<std::string>**)payload->Data;

				for (const auto& name : *selection)
					plot->addSeries(variableHandler->getVariable(name).get());
				selection->clear();
			}
			ImGui::EndDragDropTarget();
		}
	}
	plot->setIsHovered(ImGui::IsItemHovered());
}

void Gui::handleMarkers(uint32_t id, Plot::Marker& marker, ImPlotRect plotLimits, std::function<void()> activeCallback)
{
	if (marker.getState())
	{
		double markerPos = marker.getValue();
		if (markerPos == 0.0)
		{
			float offset = (std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f;
			markerPos = plotLimits.X.Min + (id == 0 ? offset : 2.0f * offset);
			marker.setValue(markerPos);
		}
		ImPlot::DragLineX(id, &markerPos, id == 0 ? ImVec4(1, 0, 0, 1) : ImVec4(0, 1, 1, 1));
		marker.setValue(markerPos);
		activeCallback();
	}
	else
		marker.setValue(0.0);
}

void Gui::handleDragRect(uint32_t id, Plot::DragRect& dragRect, ImPlotRect plotLimits)
{
	if (dragRect.getState())
	{
		auto markerPosX0 = dragRect.getValueX0();
		auto markerPosX1 = dragRect.getValueX1();

		if (markerPosX0 == 0.0 && markerPosX1 == 0.0)
		{
			float offset = (std::abs(plotLimits.X.Max) - std::abs(plotLimits.X.Min)) / 3.0f;
			markerPosX0 = plotLimits.X.Min + offset;
			markerPosX1 = plotLimits.X.Min + 2.0f * offset;
			dragRect.setValueX0(markerPosX0);
			dragRect.setValueX1(markerPosX1);
		}

		static ImPlotRect rect(0.0025, 0.0045, 0, 0.5);
		ImPlot::DragRect(id, &markerPosX0, &plotLimits.Y.Min, &markerPosX1, &plotLimits.Y.Max, ImVec4(0.15, 0.96, 0.9, 0.45), ImPlotDragToolFlags_NoFit);
		dragRect.setValueX0(markerPosX0);
		dragRect.setValueX1(markerPosX1);
	}
	else
	{
		dragRect.setValueX0(0.0);
		dragRect.setValueX1(0.0);
	}
}