#include "../gitversion.hpp"
#include "Gui.hpp"

void Gui::drawAboutWindow()
{
	if (showAboutWindow)
		ImGui::OpenPopup("About");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 300));
	if (ImGui::BeginPopupModal("About", &showAboutWindow, 0))
	{
		drawCenteredText("STMViewer");
		std::string line2("version: " + std::to_string(STMVIEWER_VERSION_MAJOR) + "." + std::to_string(STMVIEWER_VERSION_MINOR) + "." + std::to_string(STMVIEWER_VERSION_REVISION));
		drawCenteredText(std::move(line2));
		drawCenteredText(std::string(GIT_HASH));
		ImGui::SameLine();
		const bool copy = ImGui::SmallButton("copy");
		if (copy)
		{
			ImGui::LogToClipboard();
			ImGui::LogText("%s", GIT_HASH);
			ImGui::LogFinish();
		}

		ImGui::Dummy(ImVec2(-1, 20));
		drawCenteredText("by Piotr Wasilewski (klonyyy)");
		ImGui::Dummy(ImVec2(-1, 20));

		const float buttonHeight = 25.0f;

		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 210) / 2.0f);

		if (ImGui::Button("Releases", ImVec2(100, buttonHeight)))
			openWebsite("https://github.com/klonyyy/STMViewer/releases");
		ImGui::SameLine();
		if (ImGui::Button("Support <3", ImVec2(100, buttonHeight)))
			openWebsite("https://github.com/sponsors/klonyyy");

		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));
		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)))
		{
			showAboutWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

bool Gui::openWebsite(const char* url)
{
#if defined(unix) || defined(__unix__) || defined(__unix)
#define _UNIX
#endif

#ifdef _WIN32
	ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined _UNIX
	const char* browser = getenv("BROWSER");
	if (browser == NULL)
		browser = "xdg-open";
	char command[256];
	snprintf(command, sizeof(command), "%s %s", browser, url);
	system(command);
#else
#error "Your system is not supported!"
#endif
	return true;
}