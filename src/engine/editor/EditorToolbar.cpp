#include "EditorToolbar.h"
#include "core/Engine.h"
#include "editor/Editor.h"
#include "editor/EditorUI.h"

namespace eng
{
	void EditorToolbar::Draw(Editor& editor)
	{
		EditorUI::TextScope text;
		const Rectangle area = EditorUI::ToolbarRect();
		DrawRectangleRec(area, Fade(RAYWHITE, 0.9f));
		DrawRectangleLinesEx(area, 1.0f, Fade(DARKGRAY, 0.6f));

		constexpr float	GAP		= 6.0f;
		constexpr int	BUTTONS	= 7;
		const float w = (area.width - GAP * (BUTTONS + 1)) / BUTTONS;
		auto slot = [&](int i) { return Rectangle{ area.x + GAP + i * (w + GAP), area.y + 4.0f, w, area.height - 8.0f }; };

		if (GuiButton(slot(0), "Play"))
			Engine::GetInstance().GetModes().Request(EngineMode::Play);

		if (GuiButton(slot(1), "Reset"))
			editor.Reset();

		// The active gizmo mode is drawn pressed
		const GizmoMode mode = editor.GetGizmo().GetMode();
		bool isMove = mode == GizmoMode::Move;
		bool isScale = mode == GizmoMode::Scale;
		if (GuiToggle(slot(2), "Move (W)", &isMove) && isMove)		editor.GetGizmo().SetMode(GizmoMode::Move);
		if (GuiToggle(slot(3), "Scale (R)", &isScale) && isScale)	editor.GetGizmo().SetMode(GizmoMode::Scale);

		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		bool showPhysics = physics.IsDebugDrawEnabled();
		if (GuiToggle(slot(4), "Physics", &showPhysics))
			physics.SetDebugDrawEnabled(showPhysics);

		GuiSetState(editor.CanSaveLoad() ? STATE_NORMAL : STATE_DISABLED);
		if (GuiButton(slot(5), "Save"))	editor.Save();
		if (GuiButton(slot(6), "Load"))	editor.RequestLoad();
		GuiSetState(STATE_NORMAL);
	}
}
