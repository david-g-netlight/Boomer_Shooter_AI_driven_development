#include "UIStyle.h"
#include <raygui.h>

namespace eng
{
	void UIStyle::Apply()
	{
		GuiLoadStyleDefault();
		GuiSetStyle(DEFAULT, TEXT_SIZE, UIStyle::TEXT_SIZE);
		GuiSetStyle(DEFAULT, TEXT_SPACING, 2);

		// Buttons: dark slate with a light border, blue when hovered (matches the beam colors)
		GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
		GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,		ColorToInt({ 40, 48, 64, 230 }));
		GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,	ColorToInt({ 150, 170, 200, 255 }));
		GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,		ColorToInt({ 230, 235, 245, 255 }));
		GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,		ColorToInt({ 40, 110, 220, 240 }));
		GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED,	ColorToInt({ 120, 200, 255, 255 }));
		GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,		ColorToInt({ 255, 255, 255, 255 }));
		GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,		ColorToInt({ 40, 200, 120, 255 }));
		GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED,	ColorToInt({ 200, 255, 220, 255 }));
		GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,		ColorToInt({ 255, 255, 255, 255 }));
	}

	void UIStyle::DrawTitle(const char* text, float y)
	{
		const int width	= MeasureText(text, TITLE_SIZE);
		const int x		= (GetScreenWidth() - width) / 2;
		DrawText(text, x + 3, static_cast<int>(y) + 3, TITLE_SIZE, Fade(BLACK, 0.5f));
		DrawText(text, x, static_cast<int>(y), TITLE_SIZE, RAYWHITE);
	}

	Rectangle UIStyle::ButtonRect(int index, float top)
	{
		const float x = 0.5f * (static_cast<float>(GetScreenWidth()) - BUTTON_WIDTH);
		return { x, top + static_cast<float>(index) * (BUTTON_HEIGHT + BUTTON_GAP), BUTTON_WIDTH, BUTTON_HEIGHT };
	}

	void UIStyle::DimBackground(float alpha)
	{
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, alpha));
	}
}
