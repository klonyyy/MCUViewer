#ifndef _FILEHANDLER_HPP
#define _FILEHANDLER_HPP

#include <string>
#include <utility>

class IFileHandler
{
   public:
	virtual ~IFileHandler() = default;
	virtual bool init() = 0;
	virtual bool deinit() = 0;
	virtual std::string openFile(std::pair<std::string, std::string>&& filterFileNameFileExtension) = 0;
	virtual std::string saveFile(std::pair<std::string, std::string>&& filterFileNameFileExtension) = 0;
	virtual std::string openDirectory(std::pair<std::string, std::string>&& filterFileNameFileExtension) = 0;
};

#endif