#pragma once

#include "Variable.hpp"

class Recorder
{
	typedef struct Settings
	{
		const std::string state;
		const std::string timestepUs;
		const std::string maxBufferSize;
		const std::string variablesCount;
		const std::string sampleListZeroEntry;
	} Settings;

   public:
	using SampleListType = std::vector<std::pair<uint32_t, uint8_t>>;

	Recorder(VariableHandler* variableHandler) : variableHandler(variableHandler)
	{
	}

	bool init(SampleListType recorderSampleList)
	{
		if (recorderSampleList.size() == 0)
		{
			isInitialized = false;
			return false;
		}

		variableHandler->addVariable(std::make_shared<Variable>(settings.state));
		variableHandler->addVariable(std::make_shared<Variable>(settings.timestepUs));
		variableHandler->addVariable(std::make_shared<Variable>(settings.maxBufferSize));
		variableHandler->addVariable(std::make_shared<Variable>(settings.variablesCount));
		variableHandler->addVariable(std::make_shared<Variable>(settings.sampleListZeroEntry));

		variableHandler->getVariable(settings.state)->setTrackedName(settings.state);
		variableHandler->getVariable(settings.timestepUs)->setTrackedName(settings.timestepUs);
		variableHandler->getVariable(settings.maxBufferSize)->setTrackedName(settings.maxBufferSize);
		variableHandler->getVariable(settings.timestepUs)->setTrackedName(settings.variablesCount);
		variableHandler->getVariable(settings.sampleListZeroEntry)->setTrackedName(settings.sampleListZeroEntry);

		isInitialized = true;

		return true;
	}

	void start(std::shared_ptr<IDebugProbe> debugProbe, SampleListType recorderSampleList)
	{
		if (!isInitialized)
			return;

		writeVariable(debugProbe, *variableHandler->getVariable(settings.variablesCount), recorderSampleList.size());

		uint32_t addressFieldAddress = variableHandler->getVariable(settings.sampleListZeroEntry)->getAddress();
		uint32_t sizeFieldAddress = addressFieldAddress + addressFieldOffset;

		for (auto& [address, size] : recorderSampleList)
		{
			/* write address field */
			debugProbe->writeMemory(addressFieldAddress, (uint8_t*)&address, addressFieldOffset);
			debugProbe->writeMemory(sizeFieldAddress, (uint8_t*)&size, sizeFieldOffset);

			addressFieldAddress += addressFieldOffset + sizeFieldOffset;
			sizeFieldAddress += addressFieldOffset + sizeFieldOffset;
		}
	}

	void perform()
	{
	}

	void finish()
	{
	}

   private:
	bool readVariable(std::shared_ptr<IDebugProbe> debugProbe, Variable& variable, uint32_t& value)
	{
		return debugProbe->readMemory(variable.getAddress(), (uint8_t*)&value, variable.getSize());
	}

	bool writeVariable(std::shared_ptr<IDebugProbe> debugProbe, Variable& variable, uint32_t value)
	{
		return debugProbe->writeMemory(variable.getAddress(), (uint8_t*)&value, variable.getSize());
	}

   private:
	static constexpr size_t addressFieldOffset = 4;
	static constexpr size_t sizeFieldOffset = 4;

	Settings settings{
		.state{"____recorder.state"},
		.timestepUs{"____recorder.timestepUs"},
		.maxBufferSize{"____recorder.maxBufferSize"},
		.variablesCount{"____recorder.variableCount"},
		.sampleListZeroEntry{"____recorder.sampleList[0]"},
	};

	bool isInitialized = false;

	VariableHandler* variableHandler;
};