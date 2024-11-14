#include "VariableHandler.hpp"

void VariableHandler::addVariable(std::shared_ptr<Variable> var)
{
	variableMap.emplace(var->getName(), var);
}

std::shared_ptr<Variable> VariableHandler::getVariable(const std::string& name)
{
	return variableMap.at(name);
}

void VariableHandler::clear()
{
	variableMap.clear();
}

bool VariableHandler::isEmpty()
{
	return variableMap.empty();
}

void VariableHandler::erase(const std::string& name)
{
	variableMap.erase(name);
}

bool VariableHandler::contains(const std::string& name)
{
	return variableMap.find(name) != variableMap.end();
}

void VariableHandler::addNewVariable(std::string newName)
{
	bool copy = false;
	std::string originalName = newName;

	auto incrementName = [&](std::string name) -> std::string
	{
		uint32_t num = 0;
		if (name.find("_copy_") != std::string::npos)
			name = name.substr(0, name.find("_copy_"));

		while (variableMap.find(name + "_copy_" + std::to_string(num)) != variableMap.end())
			num++;

		if (name == "-new")
			return name + std::to_string(num);
		else
			return name + "_copy_" + std::to_string(num);
	};

	if (newName.empty())
		newName = incrementName("-new");
	else if (variableMap.find(newName) != variableMap.end())
	{
		copy = true;
		newName = incrementName(newName);
	}

	std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
	std::random_device rd{};
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<uint32_t> dist{0, UINT32_MAX};
	uint32_t randomColor = dist(gen);

	if (copy)
	{
		std::shared_ptr<Variable> copiedVar = variableMap.at(originalName);
		newVar->setTrackedName(copiedVar->getTrackedName());
		newVar->setAddress(copiedVar->getAddress());
		newVar->setType(copiedVar->getType());
		newVar->setShift(copiedVar->getShift());
		newVar->setHighLevelType(copiedVar->getHighLevelType());
		newVar->setIsFound(copiedVar->getIsFound());
	}
	else
		newVar->setTrackedName(newName);

	newVar->setColor(randomColor);
	variableMap.emplace(newName, newVar);
}

void VariableHandler::renameVariable(const std::string& currentName, const std::string& newName)
{
	auto temp = variableMap.extract(currentName);
	temp.key() = std::string(newName);
	variableMap.insert(std::move(temp));

	variableMap[newName]->rename(newName);

	if (renameCallback)
		renameCallback(currentName, newName);
}

VariableHandler::iterator::iterator(std::map<std::string, std::shared_ptr<Variable>>::iterator iter)
	: m_iter(iter)
{
}

VariableHandler::iterator& VariableHandler::iterator::operator++()
{
	++m_iter;
	return *this;
}

VariableHandler::iterator VariableHandler::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this);
	return tmp;
}

bool VariableHandler::iterator::operator==(const iterator& other) const
{
	return m_iter == other.m_iter;
}

bool VariableHandler::iterator::operator!=(const iterator& other) const
{
	return !(*this == other);
}

std::shared_ptr<Variable> VariableHandler::iterator::operator*()
{
	return m_iter->second;
}

VariableHandler::iterator VariableHandler::begin()
{
	return iterator(variableMap.begin());
}

VariableHandler::iterator VariableHandler::end()
{
	return iterator(variableMap.end());
}