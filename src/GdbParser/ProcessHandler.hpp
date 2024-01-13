#ifndef _PROCESSHANDLER_HPP
#define _PROCESSHANDLER_HPP

#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#endif

template <typename Platform>
class ProcessHandler
{
   public:
	std::string executeCmd(std::string cmd)
	{
		return platform.executeCmd(cmd);
	}

   private:
	Platform platform;
};

class WindowsProcessHandler
{
   public:
	~WindowsProcessHandler()
	{
		fclose(pipes.first);
		fclose(pipes.second);
	}

	std::string executeCmd(std::string cmd)
	{
		std::string result{};
		std::array<char, 128> buffer;

		if (pipes.first == nullptr || pipes.second == nullptr)
			pipes = popen2(cmd.c_str());
		else
		{
			fputs(cmd.c_str(), pipes.first);
			fflush(pipes.first);
		}

		while (fgets(buffer.data(), buffer.size(), pipes.second) != nullptr)
		{
			result += buffer.data();
			if (result.find("(gdb)") != std::string::npos)
				break;
		}

		return result;
	}

   private:
	std::pair<FILE*, FILE*> pipes;

	std::pair<FILE*, FILE*> popen2(const char* __command)
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr;

		// Create the child stdin pipe.
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
			return {nullptr, nullptr};

		// Ensure the read handle to the pipe for the child process is not inherited.
		if (!SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0))
			return {nullptr, nullptr};

		// Create the child stdout pipe.
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
			return {nullptr, nullptr};

		// Ensure the write handle to the pipe for the child process is not inherited.
		if (!SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0))
			return {nullptr, nullptr};

		PROCESS_INFORMATION piProcInfo;
		STARTUPINFO siStartInfo;

		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = hChildStdoutWr;
		siStartInfo.hStdOutput = hChildStdoutWr;
		siStartInfo.hStdInput = hChildStdinRd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		// Create the command line string.
		char commandLine[MAX_PATH];
		snprintf(commandLine, MAX_PATH, "%s", __command);

		// Create the child process.
		if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo))
			return {nullptr, nullptr};

		// Close unnecessary handles.
		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);

		// Return FILE* for child's stdin and stdout, respectively.
		return {fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(hChildStdinWr), 0), "w"),
				fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(hChildStdoutRd), 0), "r")};
	}
};

#endif