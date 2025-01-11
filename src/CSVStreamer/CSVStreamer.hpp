#ifndef _CSV_STREAMER_HPP
#define _CSV_STREAMER_HPP

#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <unordered_map>
#include <array>
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

	/// @brief Creates file in given directory with a fixed name
	/// @param directory directory string
	/// @return
	bool prepareFile(const std::string& directory);

	/// @brief create csv file header from given argument, first column - time - is added internally
	/// @param headerNames table headers
	void createHeader(const std::vector<std::string>& headerNames);

	/// @brief writes single line to internal buffer
	/// @param time
	/// @param valuesMap
	void writeLine(double time, std::unordered_map<std::string, double>& valuesMap);

	/// @brief exchanges the buffer that is being processed with the one that's being written to
	void exchangeBuffers();

	/// @brief writes the processing buffer to the opened csv file
	void writeFile();

	/// @brief writes the rest of the buffer to the file and closes it
	void finishLogging();

   private:
	const char* logFileName = "/logfile.csv";

	spdlog::logger* logger;
	std::future<void> saveTask{};
	std::string filePath;
	std::ofstream csvFile;
	Buffer buffer1{};
	Buffer buffer2{};
	Buffer* currentBuffer;
	Buffer* processingBuffer;
	std::vector<std::string> headerNames;
};

#endif