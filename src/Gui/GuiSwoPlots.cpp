#include <implot.h>

#include "Gui.hpp"

void Gui::drawPlotsSwo()
{
	ImVec2 plotSize(-1, -1);

	float rowRatios[] = {1.2f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

	if (ImPlot::BeginSubplots("##subplos", tracePlotHandler->getVisiblePlotsCount(), 1, plotSize, ImPlotSubplotFlags_LinkAllX, rowRatios))
	{
		bool first = true;
		for (std::shared_ptr<Plot> plt : *tracePlotHandler)
		{
			if (!plt->getVisibility())
				continue;

			drawPlotCurveSwo(plt.get(), plt->getTimeSeries(), plt->getSeriesMap(), first);
			first = false;
		}
		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, bool first)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(), ImPlotFlags_NoChild | ImPlotFlags_NoTitle))
	{
		if (first)
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
		else
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel);

		if (tracePlotHandler->getViewerState() == TracePlotHandler::state::RUN)
		{
			auto settings = tracePlotHandler->getSettings();
			const double min = time.getOldestValue();
			const double max = time.getNewestValue();
			const double viewportWidth = (max - min) * (settings.maxViewportPointsPercent / 100.0);
			ImPlot::SetupAxisLimits(ImAxis_X1, max - viewportWidth, max, ImPlotCond_Always);
		}
		else
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel);

		if (plot->getDomain() == Plot::Domain::DIGITAL)
		{
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel);
			ImPlot::SetupAxisLimits(ImAxis_Y1, -0.25, 1.25, ImPlotCond_Always);
		}
		else
			ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_None);

		Plot::Series* ser = plot->getSeriesMap().begin()->second.get();
		std::string serName = ser->var->getName();

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
			ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 15), true, "x1-x0 %.5f ms", dx * 1000.0);
			ImPlot::Annotation(markerPos, plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 30), true, "1/dt %.1f Hz", 1.0 / dx);
		}
		else
			plot->setMarkerValueX1(0.0);

		plot->setIsHovered(ImPlot::IsPlotHovered());

		/* make thread safe copies of buffers - TODO refactor */
		mtx->lock();
		time.copyData();
		if (ser->visible)
			ser->buffer->copyData();
		uint32_t offset = time.getOffset();
		uint32_t size = time.getSize();
		mtx->unlock();

		const double timepoint = plot->getMarkerValueX0();
		const double value = *(ser->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));

		ImPlot::SetNextLineStyle(ImVec4(ser->var->getColor().r, ser->var->getColor().g, ser->var->getColor().b, 1.0f));
		ImPlot::SetNextFillStyle(ImVec4(ser->var->getColor().r, ser->var->getColor().g, ser->var->getColor().b, 1.0f), 0.25f);
		ImPlot::PlotStairs(plot->getAlias().c_str(), time.getFirstElementCopy(), ser->buffer->getFirstElementCopy(), size, ImPlotStairsFlags_Shaded, offset, sizeof(double));

		if (plot->getMarkerStateX0())
		{
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(1, 1, 1, 1), 0.5f);
			ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
		}

		if (tracePlotHandler->getViewerState() == TracePlotHandler::state::STOP)
		{
			auto errorTimestamps = tracePlotHandler->getErrorTimestamps();
			std::vector<double> t;
			std::vector<double> values;

			for (size_t i = 0; i < errorTimestamps.size(); i++)
			{
				values.push_back(0);
				t.push_back(*(time.getFirstElementCopy() + time.getIndexFromvalue(errorTimestamps.at(i))));
			}

			ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 8, ImVec4(1, 0, 0, 1), 3, ImVec4(1, 0, 0, 1));
			ImPlot::PlotScatter("###point2", t.data(), values.data(), errorTimestamps.size(), false);
		}

		ImPlot::EndPlot();
	}
}