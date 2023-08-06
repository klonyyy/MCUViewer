#include "Gui.hpp"

void Gui::drawPlotsSwo()
{
	ImVec2 plotSize(-1, -1);

	if (ImPlot::BeginSubplots("##subplos", 5, 1, plotSize, ImPlotSubplotFlags_LinkAllX))
	{
		for (std::shared_ptr<Plot> plt : *tracePlotHandler)
		{
			if (!plt->getVisibility())
				continue;

			drawPlotCurveSwo(plt.get(), plt->getTimeSeries(), plt->getSeriesMap());
		}

		ImPlot::EndSubplots();
	}
}

void Gui::drawPlotCurveSwo(Plot* plot, ScrollingBuffer<double>& time, std::map<std::string, std::shared_ptr<Plot::Series>>& seriesMap)
{
	if (ImPlot::BeginPlot(plot->getName().c_str(), ImVec2(-1, -1), ImPlotFlags_NoChild))
	{
		if (tracePlotHandler->getViewerState() == TracePlotHandler::state::RUN)
		{
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
		}

		ImPlot::SetupAxisLimits(ImAxis_Y1, -1.5, 1.5, ImPlotCond_Always);

		Plot::Series* ser = plot->getSeriesMap().begin()->second.get();
		std::string serName = ser->var->getName();
		/* make thread safe copies of buffers - probably can be made better but it works */
		mtx->lock();
		time.copyData();
		if (ser->visible)
			ser->buffer->copyData();
		uint32_t offset = time.getOffset();
		uint32_t size = time.getSize();
		mtx->unlock();

		const double timepoint = plot->getMarkerValueX0();
		const double value = *(ser->buffer->getFirstElementCopy() + time.getIndexFromvalue(timepoint));
		auto name = plot->getMarkerStateX0() ? serName + " = " + std::to_string(value) : serName;

		ImPlot::SetNextLineStyle(ImVec4(ser->var->getColor().r, ser->var->getColor().g, ser->var->getColor().b, 1.0f), 2.0f);
		ImPlot::PlotStairs(name.c_str(), time.getFirstElementCopy(), ser->buffer->getFirstElementCopy(), size, 0, offset, sizeof(double));

		if (plot->getMarkerStateX0())
		{
			ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 3.0f, ImVec4(255, 255, 255, 255), 0.5f);
			ImPlot::PlotScatter("###point", &timepoint, &value, 1, false);
		}

		ImPlot::EndPlot();
	}
}