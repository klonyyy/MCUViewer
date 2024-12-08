#pragma once

#include "Variable.hpp"

class Recorder
{
	typedef struct Settings
	{
		const std::string state;
		const std::string timestepUs;
		const std::string maxBufferSize;
	} Settings;

   public:
	Recorder(VariableHandler* variableHandler) : variableHandler(variableHandler)
	{
	}

	void init()
	{
		variableHandler->addVariable(std::make_shared<Variable>(settings.state));
		variableHandler->addVariable(std::make_shared<Variable>(settings.timestepUs));
		variableHandler->addVariable(std::make_shared<Variable>(settings.maxBufferSize));
	}

	void perform()
	{
	}

   private:
	Settings settings{
		.state{"____recorder.state"},
		.timestepUs{"____recorder.timestepUs"},
		.maxBufferSize{"____recorder.maxBufferSize"},
	};

	VariableHandler* variableHandler;
};