
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>

#include "../commons.hpp"
#include "CLI11.hpp"
#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "NFDFileHandler.hpp"
#include "PlotHandler.hpp"
#include "VariableHandler.hpp"
#include "gitversion.hpp"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

std::atomic<bool> done = false;
std::mutex mtx;
std::shared_ptr<spdlog::logger> logger;
CLI::App app{"MCUViewer"};

void prepareLogger();
void prepareCLIParser(bool& debug, std::string& projectPath);

int main(int argc, char** argv)
{
	bool debug = false;
	std::string projectPath = "";
	prepareCLIParser(debug, projectPath);

	CLI11_PARSE(app, argc, argv);

	prepareLogger();

	if (debug)
		logger->set_level(spdlog::level::debug);
	else
		logger->set_level(spdlog::level::info);

	logger->info("Starting MCUViewer!");
	logger->info("Version: {}.{}.{}", MCUVIEWER_VERSION_MAJOR, MCUVIEWER_VERSION_MINOR, MCUVIEWER_VERSION_REVISION);
	logger->info("Commit hash {}", GIT_HASH);

	auto loggerPtr = logger.get();

	PlotGroupHandler plotGroupHandler;
	VariableHandler variableHandler;
	PlotHandler plotHandler;
	TracePlotHandler tracePlotHandler;

	ViewerDataHandler viewerDataHandler(&plotGroupHandler, &variableHandler, &plotHandler, &tracePlotHandler, done, &mtx, loggerPtr);
	TraceDataHandler traceDataHandler(&plotGroupHandler, &variableHandler, &plotHandler, &tracePlotHandler, done, &mtx, loggerPtr);

	ConfigHandler configHandler("", &plotHandler, &tracePlotHandler, &plotGroupHandler, &variableHandler, &viewerDataHandler, &traceDataHandler, loggerPtr);
	NFDFileHandler fileHandler;

	Gui gui(&plotHandler, &variableHandler, &configHandler, &plotGroupHandler, &fileHandler, &tracePlotHandler, &viewerDataHandler, &traceDataHandler, done, &mtx, loggerPtr, projectPath);

	while (!done)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	logger->info("Closing MCUViewer!");
	logger->flush();
	spdlog::shutdown();
	return 0;
}

void prepareLogger()
{
#if defined(__APPLE__) || defined(_UNIX)
	std::string logDirectory = std::string(std::getenv("HOME")) + "/MCUViewer/logs/logfile.txt";
#elif _WIN32
	std::string logDirectory = std::string(std::getenv("APPDATA")) + "/MCUViewer/logs/logfile.txt";
#else
#error "Your system is not supported!"
#endif

	spdlog::sinks_init_list sinkList = {std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
										std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDirectory, 5 * 1024 * 1024, 10)};
	logger = std::make_shared<spdlog::logger>("logger", sinkList.begin(), sinkList.end());
	spdlog::register_logger(logger);
}

void prepareCLIParser(bool& debug, std::string& projectPath)
{
	app.fallthrough();
	app.ignore_case();
	app.add_flag("-d,--debug", debug, "Use for extra debug messages and logs");
	app.add_option("-p,--project", projectPath, "Use to open a project directly from command line");
}
