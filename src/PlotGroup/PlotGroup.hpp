#pragma once

#include <map>
#include <string>

#include "Plot.hpp"

class PlotGroup
{
   public:
	PlotGroup(const std::string& name) : name(name)
	{
	}

	void addPlot(std::shared_ptr<Plot> plot)
	{
		group[plot->getName()] = plot;
	}

	void removePlot(const std::string& name)
	{
		group.erase(name);
	}

	void setName(const std::string& name)
	{
		this->name = name;
	}

	std::string getName() const
	{
		return name;
	}

	std::map<std::string, std::shared_ptr<Plot>>::const_iterator begin() const
	{
		return group.cbegin();
	}

	std::map<std::string, std::shared_ptr<Plot>>::const_iterator end() const
	{
		return group.cend();
	}

	uint32_t getVisiblePlotsCount() const
	{
		return std::count_if(group.begin(), group.end(), [](const auto& pair)
							 { return pair.second->getVisibility(); });
	}

   private:
	std::string name;
	std::map<std::string, std::shared_ptr<Plot>> group;
};

class PlotGroupHandler
{
   public:
	std::shared_ptr<PlotGroup> addGroup(const std::string& name)
	{
		project[name] = std::make_shared<PlotGroup>(name);
		return project[name];
	}

	void removeGroup(const std::string& name)
	{
		project.erase(name);
	}

	std::shared_ptr<PlotGroup> getGroup(const std::string& name)
	{
		return project.at(name);
	}

	std::map<std::string, std::shared_ptr<PlotGroup>>::const_iterator begin() const
	{
		return project.cbegin();
	}

	std::map<std::string, std::shared_ptr<PlotGroup>>::const_iterator end() const
	{
		return project.cend();
	}

	void setActiveGroup(const std::string& name)
	{
		activeGroup = name;
	}

	std::shared_ptr<PlotGroup> getActiveGroup()
	{
		return project.at(activeGroup);
	}

	bool checkIfGroupExists(const std::string&& name) const
	{
		return project.find(name) != project.end();
	}

   private:
	std::string activeGroup = "";
	std::map<std::string, std::shared_ptr<PlotGroup>> project;
};