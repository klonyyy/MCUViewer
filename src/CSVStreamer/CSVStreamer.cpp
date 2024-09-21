#include "CSVStreamer.hpp"

#include <fstream>
#include <future>
#include <iostream>
#include <string>

bool CSVStreamer::Buffer::appendLine(std::string& line)
{
	if (isFull())
	{
		return false;
	}

	buffer[index] = line;
	index++;
	return true;
}

bool CSVStreamer::Buffer::isFull() const
{
	return (index + 1) >= buffer.size();
}

CSVStreamer::CSVStreamer(spdlog::logger* logger) : logger(logger)
{
	currentBuffer = &buffer1;
	buffer1.nextBuffer = &buffer2;
	buffer2.nextBuffer = &buffer1;
}

CSVStreamer::~CSVStreamer()
{
	finishLogging();
}

bool CSVStreamer::prepareFile(std::string& directory)
{
	filePath = directory + logFileName;
	csvFile.open(filePath, std::ios::out);
	if (!csvFile.is_open())
	{
		std::cerr << "Failed to open file: " << filePath << std::endl;
		return false;
	}
	return true;
}

void CSVStreamer::createHeader(const std::vector<std::string>& values)
{
	std::string header = "time,";
	for (const auto& value : values)
	{
		header += value + ",";
	}
	header.back() = '\n';
	currentBuffer->appendLine(header);
}

void CSVStreamer::writeLine(double time, const std::vector<double>& values)
{
	std::string line = std::to_string(time) + ",";
	for (const auto& value : values)
	{
		line += std::to_string(value) + ",";
	}
	line.back() = '\n';

	currentBuffer->appendLine(line);

	if (currentBuffer->isFull())
	{
		exchangeBuffers();

		if (saveTask.valid() && saveTask.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			logger->error("Buffer overrun in CSVStreamer object!");

		saveTask = std::async(std::launch::async, &CSVStreamer::save, this);
	}
}

void CSVStreamer::exchangeBuffers()
{
	processingBuffer = currentBuffer;
	currentBuffer = currentBuffer->nextBuffer;
	currentBuffer->index = 0;
}

void CSVStreamer::save()
{
	if (!csvFile.is_open())
	{
		std::cerr << "CSV file is not open!" << std::endl;
		return;
	}

	for (size_t i = 0; i < processingBuffer->index; i++)
	{
		csvFile << processingBuffer->buffer[i];
	}
}

void CSVStreamer::finishLogging()
{
	if (csvFile.is_open())
	{
		exchangeBuffers();
		save();
		csvFile.close();
	}
}