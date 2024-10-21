#ifndef _GUI_HELPER_HPP
#define _GUI_HELPER_HPP

#include <cstdint>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include "ImguiPlugins.hpp"
#include "imgui.h"

namespace GuiHelper
{
/**
 * @brief scale of the content used in the Gui settings
 *
 */
static float contentScale = 1.0f;

/**
 * @brief Convert int32_t to hexadecilam string
 *
 * @param var
 * @return std::string
 */
std::string intToHexString(uint32_t var);

/**
 * @brief Draw window-centered text
 *
 * @param text
 */
void drawCenteredText(std::string&& text);

/**
 * @brief draw text with alignment to x fields of characters
 *
 * @param text
 * @param alignTo amount of characters to align the text to
 */
void drawTextAlignedToSize(std::string&& text, size_t alignTo);

/**
 * @brief Parse hex string to a decimal number
 *
 * @param hexStr
 * @return uint32_t result
 */
uint32_t hexStringToDecimal(const std::string& hexStr);

template <typename T>
void drawInputText(const char* id, T variable, std::function<void(std::string)> valueChanged)
{
	std::string str = std::to_string(variable);
	if (ImGui::InputText(id, &str, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL) || ImGui::IsMouseClicked(0))
		if (valueChanged)
			valueChanged(str);
}

template <typename T>
void drawDescriptionWithNumber(const char* description, T number, std::string unit = "", size_t decimalPlaces = 5, float threshold = std::nan(""), ImVec4 thresholdExceededColor = {0.0f, 0.0f, 0.0f, 1.0f})
{
	if (threshold != std::nan("") && number > threshold)
		ImGui::TextColored(thresholdExceededColor, "%s", description);
	else
		ImGui::Text("%s", description);
	ImGui::SameLine();
	std::ostringstream formattedNum;
	formattedNum << std::fixed << std::setprecision(decimalPlaces) << number;
	ImGui::Text("%s", (formattedNum.str() + unit).c_str());
}
}  // namespace GuiHelper

#endif