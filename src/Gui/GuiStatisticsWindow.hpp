#pragma once

#include "GuiHelper.hpp"
#include "GuiStatisticsWindow.hpp"
#include "Plot.hpp"
#include "Statistics.hpp"

class StatisticsWindow
{
   public:
	void drawAnalog(std::shared_ptr<Plot> plt)
	{
		std::vector<std::string> serNames{"OFF"};
		for (auto& [name, ser] : plt->getSeriesMap())
			serNames.push_back(name);

		ImGui::Text("statistics ");
		ImGui::SameLine();
		ImGui::Combo("##stats", &plt->statisticsSeries, serNames);

		if (plt->statisticsSeries != 0)
		{
			static bool selectRange = false;
			ImGui::Begin("Statistics");

			auto ser = plt->getSeries(serNames[plt->statisticsSeries]);

			ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::Text("%s", ser->var->getName().c_str());

			ImGui::Text("select range: ");
			ImGui::SameLine();
			ImGui::Checkbox("##selectrange", &selectRange);

			plt->stats.setState(selectRange);

			Statistics::AnalogResults results;
			Statistics::calculateResults(ser.get(), plt->getXAxisSeries(), plt->stats.getValueX0(), plt->stats.getValueX1(), results);

			GuiHelper::drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
			GuiHelper::drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
			GuiHelper::drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
			GuiHelper::drawDescriptionWithNumber("min:     ", results.min);
			GuiHelper::drawDescriptionWithNumber("max:     ", results.max);
			GuiHelper::drawDescriptionWithNumber("mean:    ", results.mean);
			GuiHelper::drawDescriptionWithNumber("stddev:  ", results.stddev);
			ImGui::End();
		}
		else
			plt->stats.setState(false);
	}

	void drawDigital(std::shared_ptr<Plot> plt)
	{
		std::vector<std::string> serNames{"OFF"};
		for (auto& [name, ser] : plt->getSeriesMap())
			serNames.push_back(name);

		ImGui::Text("statistics ");
		ImGui::SameLine();
		ImGui::Combo("##stats", &plt->statisticsSeries, serNames);

		if (plt->statisticsSeries != 0)
		{
			static bool selectRange = false;
			ImGui::Begin("Statistics");

			auto ser = plt->getSeries(serNames[plt->statisticsSeries]);

			ImGui::ColorEdit4("##", &ser->var->getColor().r, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::Text("%s", ser->var->getName().c_str());

			ImGui::Text("select range: ");
			ImGui::SameLine();
			ImGui::Checkbox("##selectrange", &selectRange);

			plt->stats.setState(selectRange);

			Statistics::DigitalResults results;
			Statistics::calculateResults(ser.get(), plt->getXAxisSeries(), plt->stats.getValueX0(), plt->stats.getValueX1(), results);

			GuiHelper::drawDescriptionWithNumber("t0:      ", plt->stats.getValueX0());
			GuiHelper::drawDescriptionWithNumber("t1:      ", plt->stats.getValueX1());
			GuiHelper::drawDescriptionWithNumber("t1-t0:   ", plt->stats.getValueX1() - plt->stats.getValueX0());
			GuiHelper::drawDescriptionWithNumber("Lmin:    ", results.Lmin);
			GuiHelper::drawDescriptionWithNumber("Lmax:    ", results.Lmax);
			GuiHelper::drawDescriptionWithNumber("Hmin:    ", results.Hmin);
			GuiHelper::drawDescriptionWithNumber("Hmax:    ", results.Hmax);
			GuiHelper::drawDescriptionWithNumber("fmin:    ", results.fmin);
			GuiHelper::drawDescriptionWithNumber("fmax:    ", results.fmax);
			ImGui::End();
		}
		else
			plt->stats.setState(false);
	}
};