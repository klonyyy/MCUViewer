#ifndef _PROCESSHANDLER_HPP
#define _PROCESSHANDLER_HPP

#include <string>
#include <utility>

template <typename Platform>
class ProcessHandler
{
   public:
	std::string executeCmd(std::string cmd, const std::string& endMarker)
	{
		return platform.executeCmd(cmd, endMarker);
	}
	void closePipes()
	{
		platform.closePipes();
	}

   private:
	Platform platform;
};

#ifdef _WIN32
#include <io.h>
#include <windows.h>

class WindowsProcessHandler
{
   public:
	~WindowsProcessHandler()
	{
		closePipes();
	}

	void closePipes()
	{
		if (pipes.first != nullptr)
		{
			fclose(pipes.first);
			pipes.first = nullptr;
		}

		if (pipes.second != nullptr)
		{
			fclose(pipes.second);
			pipes.second = nullptr;
		}

		if (pipes.first != nullptr && pipes.second != nullptr)
		{
			CloseHandle((HANDLE)_get_osfhandle(_fileno(pipes.first)));
			CloseHandle((HANDLE)_get_osfhandle(_fileno(pipes.second)));
		}
	}

	std::string executeCmd(std::string cmd, const std::string& endMarker)
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
			if (result.find(endMarker) != std::string::npos)
				break;
		}

		return result;
	}

   private:
	std::pair<FILE*, FILE*> pipes{nullptr, nullptr};

	std::pair<FILE*, FILE*> popen2(const char* __command)
	{
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr;

		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
			return {nullptr, nullptr};
		if (!SetHandleInformation(hChildStdinWr, HANDLE_FLAG_INHERIT, 0))
			return {nullptr, nullptr};

		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
			return {nullptr, nullptr};
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
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		siStartInfo.wShowWindow = SW_HIDE;

		char commandLine[MAX_PATH];
		snprintf(commandLine, MAX_PATH, "%s", __command);

		if (!CreateProcess(NULL, commandLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo))
			return {nullptr, nullptr};

		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);

		return {fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(hChildStdinWr), 0), "w"),
				fdopen(_open_osfhandle(reinterpret_cast<intptr_t>(hChildStdoutRd), 0), "r")};
	}
};
#endif

#if defined(unix) || defined(__unix__) || defined(__unix)
#define _UNIX
#endif

#ifdef _UNIX
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class UnixProcessHandler
{
   public:
	~UnixProcessHandler()
	{
		closePipes();
	}

	void closePipes()
	{
		if (pipes.first != nullptr)
		{
			fclose(pipes.first);
			pipes.first = nullptr;
		}

		if (pipes.second != nullptr)
		{
			fclose(pipes.second);
			pipes.second = nullptr;
		}
	}

	std::string executeCmd(std::string cmd, const std::string& endMarker)
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
			if (result.find(endMarker) != std::string::npos)
				break;
		}

		return result;
	}

   private:
	std::pair<FILE*, FILE*> pipes{nullptr, nullptr};

	std::pair<FILE*, FILE*> popen2(const char* command)
	{
		int inpipefd[2];
		int outpipefd[2];

		if (pipe(inpipefd) == -1 || pipe(outpipefd) == -1)
		{
			return {nullptr, nullptr};
		}

		pid_t pid = fork();

		if (pid == -1)
		{
			close(inpipefd[0]);
			close(inpipefd[1]);
			close(outpipefd[0]);
			close(outpipefd[1]);
			return {nullptr, nullptr};
		}
		else if (pid == 0)
		{
			close(inpipefd[1]);
			close(outpipefd[0]);

			dup2(inpipefd[0], STDIN_FILENO);
			dup2(outpipefd[1], STDOUT_FILENO);

			close(inpipefd[0]);
			close(outpipefd[1]);

			execl("/bin/sh", "/bin/sh", "-c", command, (char*)NULL);
			perror("execl");
			_exit(1);
		}
		else
		{
			close(inpipefd[0]);
			close(outpipefd[1]);
			return {fdopen(inpipefd[1], "w"), fdopen(outpipefd[0], "r")};
		}
	}
};

#endif

#endif