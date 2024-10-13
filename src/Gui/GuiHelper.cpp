#include "GuiHelper.hpp"

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
