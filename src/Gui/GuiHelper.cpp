#include "GuiHelper.hpp"

#include <filesystem>

std::string GuiHelper::intToHexString(uint32_t var)
{
	std::stringstream ss;
	ss << std::hex << var;
	return ss.str();
}

void GuiHelper::drawCenteredText(std::string&& text)
{
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text.c_str()).x) * 0.5f);
	ImGui::Text("%s", text.c_str());
}

void GuiHelper::drawTextAlignedToSize(std::string&& text, size_t alignTo)
{
	size_t currentLength = text.length();
	size_t spacesToAdd = (currentLength < alignTo) ? (alignTo - currentLength) : 0;
	std::string alignedText = text + std::string(spacesToAdd, ' ');
	ImGui::Text("%s", alignedText.c_str());
}

uint32_t GuiHelper::hexStringToDecimal(const std::string& hexStr)
{
	std::string cleanHexStr = hexStr;

	if (hexStr.size() >= 2 && (hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X')))
		cleanHexStr = hexStr.substr(2);

	uint32_t decimalValue;
	std::stringstream ss;
	ss << std::hex << cleanHexStr;
	ss >> decimalValue;

	return decimalValue;
}


std::string GuiHelper::convertProjectPathToAbsolute(const std::string* relativePath, std::string* projectConfigPath)
{
	if (relativePath->empty())
		return "";

	try
	{
		// Convert relative path to absolute path based on project file location
		std::filesystem::path absPath = std::filesystem::absolute(std::filesystem::path(*projectConfigPath).parent_path() / (*relativePath));
		return absPath.string();
	}
	catch (std::filesystem::filesystem_error& e)
	{
		// logger->error("Failed to convert path to absolute: {}", e.what()); // TODO
		return "";
	}
}

std::optional<std::string> GuiHelper::showDeletePopup(const char* text, const std::string& name)
{
	ImGui::PushID(name.c_str());
	if (ImGui::BeginPopupContextItem(text))
	{
		if (ImGui::Button(text))
		{
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			ImGui::PopID();
			return name;
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return std::nullopt;
}

void GuiHelper::showQuestionBox(const char* id, const char* question, std::function<void()> onYes, std::function<void()> onNo, std::function<void()> onCancel)
{
	float buttonWidth = 120.0f * GuiHelper::contentScale;
	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(id, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("%s", question);
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(buttonWidth, 0)))
		{
			onYes();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(buttonWidth, 0)))
		{
			onNo();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
		{
			onCancel();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
