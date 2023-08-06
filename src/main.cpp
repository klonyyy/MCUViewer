
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

	spdlog::sinks_init_list sinkList = {std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
										std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDirectory, 5 * 1024 * 1024, 10)};
	logger = std::make_shared<spdlog::logger>("logger", sinkList.begin(), sinkList.end());
	spdlog::register_logger(logger);

	if (args.size() >= 2 && args.at(1) == "-d")
		logger->set_level(spdlog::level::debug);
	else
		logger->set_level(spdlog::level::info);

	logger->info("Starting STMViewer!");
	logger->info("Version: {}.{}.{}", STMVIEWER_VERSION_MAJOR, STMVIEWER_VERSION_MINOR, STMVIEWER_VERSION_REVISION);
	logger->info("Commit hash {}", GIT_HASH);

	PlotHandler plotHandler(done, &mtx, logger);
	TracePlotHandler tracePlotHandler(done, &mtx, logger);
	ConfigHandler configHandler("", &plotHandler, logger);
	NFDFileHandler fileHandler;

	tracePlotHandler.addPlot("CH0");
	Variable var1("CH0");
	var1.setColor(378440825);
	tracePlotHandler.getPlot("CH0")->addSeries(var1);
	Variable var2("CH1");
	var2.setColor(378440825);
	tracePlotHandler.addPlot("CH1");
	tracePlotHandler.getPlot("CH1")->addSeries(var2);

	tracePlotHandler.addPlot("CH2");
	Variable var3("CH2");
	var3.setColor(378440825);
	tracePlotHandler.getPlot("CH2")->addSeries(var3);
	Variable var4("CH3");
	var4.setColor(378440825);
	tracePlotHandler.addPlot("CH3");
	tracePlotHandler.getPlot("CH3")->addSeries(var4);

	Gui gui(&plotHandler, &configHandler, &fileHandler, &tracePlotHandler, done, &mtx, logger);

	while (!done)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	logger->info("Closing STMViewer!");
	logger->flush();
	spdlog::shutdown();
	return 0;
}
