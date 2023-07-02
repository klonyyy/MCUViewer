#include <NFDFileHandler.hpp>
#include <algorithm>

#include "nfd.h"

bool NFDFileHandler::init()
{
	return NFD_Init() != NFD_ERROR;
}
bool NFDFileHandler::deinit()
{
	NFD_Quit();
	return true;
}
std::string NFDFileHandler::openFile(std::pair<std::string, std::string>&& filterFileNameFileExtension)
{
	return handleFile(handleType::OPEN, filterFileNameFileExtension);
}
std::string NFDFileHandler::saveFile(std::pair<std::string, std::string>&& filterFileNameFileExtension)
{
	return handleFile(handleType::SAVE, filterFileNameFileExtension);
}

std::string NFDFileHandler::handleFile(handleType type, std::pair<std::string, std::string>& filterFileNameFileExtension)
{
	nfdchar_t* outPath = nullptr;
	nfdfilteritem_t filterItem[1] = {{filterFileNameFileExtension.first.c_str(), filterFileNameFileExtension.second.c_str()}};

	nfdresult_t result = NFD_ERROR;

	if (type == handleType::SAVE)
		result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
	else if (type == handleType::OPEN)
		result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);

	if (result == NFD_OKAY)
	{
		std::string path = std::string(outPath);
		std::replace(path.begin(), path.end(), '\\', '/');
		NFD_FreePath(outPath);
		return path;
	}
	return std::string("");
}