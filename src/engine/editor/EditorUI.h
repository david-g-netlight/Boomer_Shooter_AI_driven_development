#pragma once
#include <raylib.h>
#include <raygui.h>

namespace eng::EditorUI
{
	constexpr float	TOP_BAR_HEIGHT		= 36.0f;	// the mode banner
	constexpr float	TOOLBAR_HEIGHT		= 34.0f;
	constexpr float	HIERARCHY_WIDTH		= 250.0f;
	constexpr float	INSPECTOR_WIDTH		= 320.0f;
	constexpr float	ROW_HEIGHT			= 26.0f;
	constexpr float	PADDING				= 8.0f;
	constexpr int	TEXT_SIZE			= 16;

	/// <summary>
	/// Panel rectangles in screen points, recomputed every call so they follow window resizes.
	/// </summary>
	inline Rectangle HierarchyRect()
	{
		return { 0.0f, TOP_BAR_HEIGHT, HIERARCHY_WIDTH, static_cast<float>(GetScreenHeight()) - TOP_BAR_HEIGHT };
	}

	inline Rectangle InspectorRect()
	{
		const float w = static_cast<float>(GetScreenWidth());
		return { w - INSPECTOR_WIDTH, TOP_BAR_HEIGHT, INSPECTOR_WIDTH, static_cast<float>(GetScreenHeight()) - TOP_BAR_HEIGHT };
	}

	inline Rectangle ToolbarRect()
	{
		const float x = HIERARCHY_WIDTH;
		return { x, TOP_BAR_HEIGHT, static_cast<float>(GetScreenWidth()) - HIERARCHY_WIDTH - INSPECTOR_WIDTH, TOOLBAR_HEIGHT };
	}

	/// <summary>
	/// True when the point is over any editor panel (so clicks there must not pick or drag in the 3D view).
	/// </summary>
	inline bool IsOverPanels(const Vector2& point)
	{
		return CheckCollisionPointRec(point, HierarchyRect()) || CheckCollisionPointRec(point, InspectorRect())
			|| CheckCollisionPointRec(point, ToolbarRect()) || point.y < TOP_BAR_HEIGHT;
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
