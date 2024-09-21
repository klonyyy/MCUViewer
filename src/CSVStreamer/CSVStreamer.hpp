#ifndef _CSV_STREAMER_HPP
#define _CSV_STREAMER_HPP

#include <fstream>
#include <future>
#include <iostream>
#include <string>

#include "spdlog/spdlog.h"

class CSVStreamer
{
   public:
	struct Buffer
	{
		bool appendLine(std::string& line);
		bool isFull() const;

		std::array<std::string, 1000> buffer;
		size_t index;
		Buffer* nextBuffer;
	};

	CSVStreamer(spdlog::logger* logger);

	~CSVStreamer();

	bool prepareFile(std::string& directory);

	void createHeader(const std::vector<std::string>& values);

	void writeLine(double time, const std::vector<double>& values);

	void exchangeBuffers();

	void save();

	void finishLogging();

   private:
	const char* logFileName = "/logfile.csv";

	spdlog::logger* logger;
	std::future<void>
		saveTask{};
	std::string filePath;
	std::ofstream csvFile;
	Buffer buffer1;
	Buffer buffer2;
	Buffer* currentBuffer;
	Buffer* processingBuffer;
};

#endif