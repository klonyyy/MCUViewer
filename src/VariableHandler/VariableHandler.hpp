#pragma once

#include <map>
#include <memory>
#include <random>

#include "Variable.hpp"

class VariableHandler
{
   public:
	using VariableMap = std::map<std::string, std::shared_ptr<Variable>>;

   public:
	void addVariable(std::shared_ptr<Variable> var);

	std::shared_ptr<Variable> getVariable(const std::string& name);

	void clear();

	bool isEmpty();

	void erase(const std::string& name);

	bool contains(const std::string& name);

	void addNewVariable(std::string newName);

	void renameVariable(const std::string& currentName, const std::string& newName);

	class iterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		explicit iterator(std::map<std::string, std::shared_ptr<Variable>>::iterator iter);
		iterator& operator++();
		iterator operator++(int);
		bool operator==(const iterator& other) const;
		bool operator!=(const iterator& other) const;
		std::shared_ptr<Variable> operator*();

	   private:
		std::map<std::string, std::shared_ptr<Variable>>::iterator m_iter;
	};

	iterator begin();
	iterator end();

   private:
	VariableMap variableMap;
};