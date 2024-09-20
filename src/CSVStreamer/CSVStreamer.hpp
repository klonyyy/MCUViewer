#ifndef _CSV_STREAMER_HPP
#define _CSV_STREAMER_HPP

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

class CSVStreamer
{
   public:
	struct Buffer
	{
		std::array<std::string, 1000> buffer;
		size_t index;
		Buffer* nextBuffer;

		bool appendLine(std::string& line)
		{
			if (isFull())
			{
				return false;
			}

			buffer[index] = line;
			index++;
			return true;
		}

		bool isFull() const
		{
			return (index + 1) >= buffer.size();
		}
	};

	CSVStreamer()
	{
		currentBuffer = &buffer1;
		buffer1.nextBuffer = &buffer2;
		buffer2.nextBuffer = &buffer1;
	}

	~CSVStreamer()
	{
		finishLogging();
	}

	std::string getFilePath() const
	{
		return filePath;
	}

	bool prepareFile(std::string& directory)
	{
		filePath = directory + "/logfile.csv";
		csvFile.open(filePath, std::ios::out);
		if (!csvFile.is_open())
		{
			std::cerr << "Failed to open file: " << filePath << std::endl;
			return false;
		}
		return true;
	}

	void createHeader(const std::vector<std::string>& values)
	{
		std::string header = "time,";
		for (const auto& value : values)
		{
			header += value + ",";
		}
		header.back() = '\n';
		currentBuffer->appendLine(header);
	}

	void writeLine(double time, const std::vector<double>& values)
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
			saveTask = std::async(std::launch::async, &CSVStreamer::save, this);
		}
	}

	void exchangeBuffers()
	{
		processingBuffer = currentBuffer;
		currentBuffer = currentBuffer->nextBuffer;
		currentBuffer->index = 0;
	}

	void save()
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

	void finishLogging()
	{
		if (csvFile.is_open())
		{
			exchangeBuffers();
			save();
			csvFile.close();
		}
	}

   private:
	std::future<void> saveTask{};
	std::string filePath;
	std::ofstream csvFile;
	Buffer buffer1;
	Buffer buffer2;
	Buffer* currentBuffer;
	Buffer* processingBuffer;
};

#endif