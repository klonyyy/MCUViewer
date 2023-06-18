
#include <spdlog/sinks/stdout_color_sinks.h>

#include <iostream>

#include "ConfigHandler.hpp"
#include "Gui.hpp"
#include "NFDFileHandler.hpp"
#include "PlotHandler.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

bool done = false;
std::mutex mtx;

int main(int ac, char** av)
{
	spdlog::sinks_init_list sinkList = {std::make_shared<spdlog::sinks::stdout_color_sink_st>(),
										std::make_shared<spdlog::sinks::basic_file_sink_mt>("logfile.txt", true)};
	std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("logger", sinkList.begin(), sinkList.end());

	logger->info("Starting STMViewer!");

	PlotHandler plotHandler(done, &mtx, logger);
	ConfigHandler configHandler("", &plotHandler, logger);
	NFDFileHandler fileHandler;
	Gui gui(&plotHandler, &configHandler, &fileHandler, done, &mtx, logger);

	while (!done)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	logger->info("Closing STMViewer!");
	spdlog::shutdown();
	return 0;
}
