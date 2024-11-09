#pragma once

#include <algorithm>
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

	bool renamePlot(const std::string& oldName, const std::string& newName)
	{
		if (!group.contains(oldName))
			return false;

		auto plot = group.extract(oldName);
		plot.key() = newName;
		group.insert(std::move(plot));
		return true;
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
		groupMap[name] = std::make_shared<PlotGroup>(name);
		return groupMap[name];
	}

	void renameGroup(const std::string& oldName, const std::string& newName)
	{
		auto group = groupMap.extract(oldName);
		group.key() = newName;
		groupMap.insert(std::move(group));
		groupMap[newName]->setName(newName);
	}

	void removeGroup(const std::string& name)
	{
		groupMap.erase(name);

		if (groupMap.size() == 0)
			addGroup("new group0");

		activeGroup = groupMap.begin()->first;
	}

	void removeAllGroups()
	{
		groupMap.clear();
	}

	bool renamePlotInAllGroups(const std::string& oldName, const std::string& newName)
	{
		for (auto& [name, group] : groupMap)
		{
			group->renamePlot(oldName, newName);
		}
		return true;
	}

	size_t getGroupCount()
	{
		return groupMap.size();
	}

	std::shared_ptr<PlotGroup> getGroup(const std::string& name)
	{
		return groupMap.at(name);
	}

	std::map<std::string, std::shared_ptr<PlotGroup>>::const_iterator begin() const
	{
		return groupMap.cbegin();
	}

	std::map<std::string, std::shared_ptr<PlotGroup>>::const_iterator end() const
	{
		return groupMap.cend();
	}

	void setActiveGroup(const std::string& name)
	{
		activeGroup = name;
	}

	std::shared_ptr<PlotGroup> getActiveGroup()
	{
		if (!groupMap.contains(activeGroup))
			activeGroup = groupMap.begin()->first;
		return groupMap.at(activeGroup);
	}

	bool checkIfGroupExists(const std::string& name) const
	{
		return groupMap.find(name) != groupMap.end();
	}

   private:
	std::string activeGroup = "";
	std::map<std::string, std::shared_ptr<PlotGroup>> groupMap;
};