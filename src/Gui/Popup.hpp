#ifndef POPUP_HPP_
#define POPUP_HPP_

#include <chrono>
#include <string>

#include "imgui.h"

class Popup
{
   public:
	void show(const char* title, const char* msg, float showTime)
	{
		this->title = title;
		this->msg = msg;
		this->showTime = showTime;
		if (running)
			return;
		start = std::chrono::high_resolution_clock::now();
		running = true;
	}

	void handle()
	{
		auto popupTimer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

		if (running)
			ImGui::OpenPopup(title.c_str());

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(msg.c_str());

			if (popupTimer.count() / 1000.0f >= showTime)
			{
				ImGui::CloseCurrentPopup();
				running = false;
			}

			ImGui::EndPopup();
		}
	}

   private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::string title;
	std::string msg;
	float showTime = 0;
	bool running = false;
};

#endif