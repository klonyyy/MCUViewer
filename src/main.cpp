
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>

#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "NFDFileHandler.hpp"
#include "PlotHandler.hpp"
#include "gitversion.hpp"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

#if defined(unix) || defined(__unix__) || defined(__unix)
#define _UNIX
#endif

bool done = false;
std::mutex mtx;
std::shared_ptr<spdlog::logger> logger;

int main(int argc, char** argv)
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);

#ifdef _UNIX
	std::string logDirectory = std::string(std::getenv("HOME")) + "/STMViewer/logs/logfile.txt";
#elif _WIN32
	std::string logDirectory = std::string(std::getenv("APPDATA")) + "/STMViewer/logs/logfile.txt";
#else
#error "Your system is not supported!"
#endif

	spdlog::sinks_init_list sinkList = {std::make_shared<spdlog::sinks::stdout_color_sink_st>(),
										std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDirectory, 5 * 1024 * 1024, 10)};
	logger = std::make_shared<spdlog::logger>("logger", sinkList.begin(), sinkList.end());

	if (args.size() >= 2 && args.at(1) == "-d")
		logger->set_level(spdlog::level::debug);
	else
		logger->set_level(spdlog::level::info);

	spdlog::flush_every(std::chrono::milliseconds(500));

	logger->info("Starting STMViewer!");
	logger->info("Version: {}.{}.{}", STMVIEWER_VERSION_MAJOR, STMVIEWER_VERSION_MINOR, STMVIEWER_VERSION_REVISION);
	logger->info("Commit hash {}", GIT_HASH);

	PlotHandler plotHandler(done, &mtx, logger);
	ConfigHandler configHandler("", &plotHandler, logger);
	NFDFileHandler fileHandler;
	Gui gui(&plotHandler, &configHandler, &fileHandler, done, &mtx, logger);

	while (!done)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	logger->info("Closing STMViewer!");
	logger->flush();
	spdlog::shutdown();
	return 0;
}
