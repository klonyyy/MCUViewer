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
	if (newName.empty())
	{
		uint32_t num = 0;
		while (variableMap.find(std::string("-new") + std::to_string(num)) != variableMap.end())
			num++;

		newName = std::string("-new") + std::to_string(num);
	}

	std::shared_ptr<Variable> newVar = std::make_shared<Variable>(newName);
	std::random_device rd{};
	std::mt19937 gen{rd()};
	std::uniform_int_distribution<uint32_t> dist{0, UINT32_MAX};
	uint32_t randomColor = dist(gen);
	newVar->setColor(randomColor);
	newVar->renameCallback = [&](const std::string& currentName, const std::string& newName)
	{
		renameVariable(currentName, newName);
	};
	newVar->setTrackedName(newName);
	variableMap.emplace(newName, newVar);
}

void VariableHandler::renameVariable(const std::string& currentName, const std::string& newName)
{
	auto temp = variableMap.extract(currentName);
	temp.key() = std::string(newName);
	variableMap.insert(std::move(temp));
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