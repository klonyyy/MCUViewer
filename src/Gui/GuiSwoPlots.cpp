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

			drawPlotCurveSwo(plt.get(), *plt->getXAxisSeries(), plt->getSeriesMap(), first);
			first = false;
		}
		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap, bool first)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(), ImPlotFlags_NoTitle))
	{
		if (first)
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoLabel);
		else
			ImPlot::SetupAxis(ImAxis_X1, "time[s]", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel);

		if (traceDataHandler->getState() == DataHandlerBase::State::RUN)
		{
			auto settings = traceDataHandler->getSettings();
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

		if (plot->trigger.getState())
		{
			auto triggerLevel = plot->trigger.getValue();
			ImPlot::DragLineY(0, &triggerLevel, ImVec4(1.0, 0.9, 0.0, 1.0), 1.0f);
			plot->trigger.setValue(triggerLevel);
		}

		if (traceDataHandler->getState() == DataHandlerBase::State::STOP)
		{
			ImPlotRect plotLimits = ImPlot::GetPlotLimits();
			handleMarkers(0, plot->markerX0, plotLimits, [&]()
						  { ImPlot::Annotation(plot->markerX0.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(-10, 0), true, "x0 %.5f", plot->markerX0.getValue()); });
			handleMarkers(1, plot->markerX1, plotLimits, [&]()
						  {
			ImPlot::Annotation(plot->markerX1.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 0), true, "x1 %.5f", plot->markerX1.getValue());
			double dx = plot->markerX1.getValue() - plot->markerX0.getValue();
			ImPlot::Annotation(plot->markerX1.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 15), true, "x1-x0 %.5f ms", dx * 1000.0);
			ImPlot::Annotation(plot->markerX1.getValue(), plotLimits.Y.Max, ImVec4(0, 0, 0, 0), ImVec2(10, 30), true, "1/dt %.1f Hz", 1.0 / dx); });

			handleDragRect(0, plot->stats, plotLimits);
		}

		plot->setIsHovered(ImPlot::IsPlotHovered());

		/* make thread safe copies of buffers - TODO refactor */
		mtx->lock();
		time.copyData();
		if (ser->visible)
			ser->buffer->copyData();
		uint32_t offset = time.getOffset();
		uint32_t size = time.getSize();
		mtx->unlock();

		const double timepoint = plot->markerX0.getValue();
		const double value = *(ser->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));

		ImPlot::SetNextLineStyle(ImVec4(ser->var->getColor().r, ser->var->getColor().g, ser->var->getColor().b, 1.0f));
		ImPlot::SetNextFillStyle(ImVec4(ser->var->getColor().r, ser->var->getColor().g, ser->var->getColor().b, 1.0f), 0.25f);
		ImPlot::PlotStairs(plot->getAlias().c_str(), time.getFirstElementCopy(), ser->buffer->getFirstElementCopy(), size, ImPlotStairsFlags_Shaded, offset, sizeof(double));

		if (plot->markerX0.getState())
		{
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(1, 1, 1, 1), 0.5f);
			ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
		}

		if (traceDataHandler->getState() == DataHandlerBase::State::STOP)
		{
			auto errorTimestamps = traceDataHandler->getErrorTimestamps();
			auto delayed3Timestamps = traceDataHandler->getDelayed3Timestamps();
			std::array<double, 100> values{0};

			ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 8, ImVec4(1, 0, 0, 1), 3, ImVec4(1, 0, 0, 1));
			ImPlot::PlotScatter("###point2", errorTimestamps.data(), values.data(), errorTimestamps.size(), false);

			ImPlot::SetNextMarkerStyle(ImPlotMarker_Cross, 6, ImVec4(1, 1, 0, 1), 3, ImVec4(1, 1, 0, 1));
			ImPlot::PlotScatter("###point3", delayed3Timestamps.data(), values.data(), delayed3Timestamps.size(), false);
		}

		ImPlot::EndPlot();
	}
}