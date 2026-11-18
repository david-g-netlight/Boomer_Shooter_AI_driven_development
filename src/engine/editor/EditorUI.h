#pragma once
#include <raylib.h>
#include <raygui.h>

namespace eng::EditorUI
{
	constexpr float	TOP_BAR_HEIGHT		= 36.0f;	// the status line
	constexpr float	TOOLBAR_HEIGHT		= 34.0f;
	constexpr int	TEXT_SIZE			= 16;

	/// <summary>
	/// The toolbar strip under the status line, recomputed every call so it follows window resizes.
	/// </summary>
	inline Rectangle ToolbarRect()
	{
		return { 0.0f, TOP_BAR_HEIGHT, static_cast<float>(GetScreenWidth()), TOOLBAR_HEIGHT };
	}

	/// <summary>
	/// True when the point is over the status line or the toolbar (so clicks there must not pick or drag in the 3D view).
	/// </summary>
	inline bool IsOverToolbar(const Vector2& point)
	{
		return CheckCollisionPointRec(point, ToolbarRect()) || point.y < TOP_BAR_HEIGHT;
	}

	/// <summary>
	/// Smaller raygui text while the editor panels are drawn (the menus use a bigger size). Restores on exit.
	/// </summary>
	struct TextScope
	{
		TextScope()		{ m_previous = GuiGetStyle(DEFAULT, TEXT_SIZE); GuiSetStyle(DEFAULT, TEXT_SIZE, TEXT_SIZE); }
		~TextScope()	{ GuiSetStyle(DEFAULT, TEXT_SIZE, m_previous); }
		TextScope(const TextScope&) = delete;
		TextScope& operator = (const TextScope&) = delete;
	private:
		int m_previous = 0;
	};
}
