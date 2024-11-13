#include "../commons.hpp"
#include "../gitversion.hpp"
#include "Gui.hpp"

#ifdef _WIN32
#include <shellapi.h>
#endif

void Gui::drawAboutWindow()
{
	if (showAboutWindow)
		ImGui::OpenPopup("About");

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500 * GuiHelper::contentScale, 300 * GuiHelper::contentScale));
	if (ImGui::BeginPopupModal("About", &showAboutWindow, 0))
	{
		GuiHelper::drawCenteredText("MCUViewer");
		std::string line2("version: " + std::to_string(MCUVIEWER_VERSION_MAJOR) + "." + std::to_string(MCUVIEWER_VERSION_MINOR) + "." + std::to_string(MCUVIEWER_VERSION_REVISION));
		GuiHelper::drawCenteredText(std::move(line2));
		GuiHelper::drawCenteredText(std::string(GIT_HASH));
		ImGui::SameLine();
		const bool copy = ImGui::SmallButton("copy");
		if (copy)
		{
			ImGui::LogToClipboard();
			ImGui::LogText("%s", GIT_HASH);
			ImGui::LogFinish();
		}

		ImGui::Dummy(ImVec2(-1, 20 * GuiHelper::contentScale));
		GuiHelper::drawCenteredText("by Piotr Wasilewski (klonyyy)");
		ImGui::Dummy(ImVec2(-1, 20 * GuiHelper::contentScale));

		const float buttonHeight = 25.0f * GuiHelper::contentScale;

		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 210 * GuiHelper::contentScale) / 2.0f);

		if (ImGui::Button("Releases", ImVec2(100 * GuiHelper::contentScale, buttonHeight)))
			openWebsite("https://github.com/klonyyy/MCUViewer/releases");
		ImGui::SameLine();
		if (ImGui::Button("Support <3", ImVec2(100 * GuiHelper::contentScale, buttonHeight)))
			openWebsite("https://github.com/sponsors/klonyyy");

		ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowSize().y - buttonHeight / 2.0f - ImGui::GetFrameHeightWithSpacing()));
		if (ImGui::Button("Done", ImVec2(-1, buttonHeight)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showAboutWindow = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

bool Gui::openWebsite(const char* url)
{
#ifdef _WIN32
	ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__) || defined(_UNIX)
	const char* browser = getenv("BROWSER");
	if (browser == NULL)
		browser = "xdg-open";
	char command[256];
	snprintf(command, sizeof(command), "%s %s", browser, url);
	// auto status = system(command);
#else
#error "Your system is not supported!"
#endif
	return true;
}
