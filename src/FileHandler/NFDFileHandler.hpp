#ifndef _NFDFILEHANDLER_HPP
#define _NFDFILEHANDLER_HPP

#include <IFileHandler.hpp>

class NFDFileHandler : public IFileHandler
{
   public:
	bool
	init() override;
	bool deinit() override;
	std::string openFile(std::pair<std::string, std::string>&& filterFileNameFileExtension) override;
	std::string saveFile(std::pair<std::string, std::string>&& filterFileNameFileExtension) override;

   private:
	enum class handleType
	{
		SAVE,
		OPEN
	};
	std::string handleFile(handleType type, std::pair<std::string, std::string>& filterFileNameFileExtension);
};

#endif