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

		variableHandler->getVariable(settings.state)->setTrackedName(settings.state);
		variableHandler->getVariable(settings.timestepUs)->setTrackedName(settings.timestepUs);
		variableHandler->getVariable(settings.maxBufferSize)->setTrackedName(settings.maxBufferSize);
	}

	void start()
	{
	}

	void perform()
	{
	}

	void finish()
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