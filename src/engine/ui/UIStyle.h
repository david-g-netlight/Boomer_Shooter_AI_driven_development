#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Look of every raygui screen, plus small layout helpers (screen coordinates, points).
	/// Apply() is called by the Engine once the window exists.
	/// </summary>
	class UIStyle
	{
	public:
		static constexpr int	TEXT_SIZE		= 20;
		static constexpr int	TITLE_SIZE		= 48;
		static constexpr float	BUTTON_WIDTH	= 260.0f;
		static constexpr float	BUTTON_HEIGHT	= 44.0f;
		static constexpr float	BUTTON_GAP		= 14.0f;

		static void			Apply();

		/// <summary>
		/// Big centered title with a soft shadow, `y` from the top of the screen.
		/// </summary>
		static void			DrawTitle(const char* text, float y);

		/// <summary>
		/// Centered column of buttons: rectangle of button `index` (0-based) starting at `top`.
		/// </summary>
		static Rectangle	ButtonRect(int index, float top);

		/// <summary>
		/// Darkens the whole screen (behind dialogs).
		/// </summary>
		static void			DimBackground(float alpha);
	};
}
