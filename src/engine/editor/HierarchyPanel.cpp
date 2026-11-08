#include "HierarchyPanel.h"
#include <cstring>
#include "editor/Editor.h"
#include "editor/EditorUI.h"
#include "level/GameActor.h"
#include "level/Level.h"

namespace eng
{
	bool HierarchyPanel::IsEditingText() const { return m_isRenaming; }

	void HierarchyPanel::Draw(Editor& editor, Level& level)
	{
		EditorUI::TextScope text;
		const Rectangle area = EditorUI::HierarchyRect();
		GuiPanel(area, "Hierarchy");

		const float x = area.x + EditorUI::PADDING;
		const float w = area.width - 2.0f * EditorUI::PADDING;
		float y = area.y + 28.0f;

		// Add / delete
		const float half = (w - EditorUI::PADDING) * 0.5f;
		if (GuiButton({ x, y, half, EditorUI::ROW_HEIGHT }, "Add Cube"))
			editor.AddCube();
		if (GuiButton({ x + half + EditorUI::PADDING, y, half, EditorUI::ROW_HEIGHT }, "Add Trigger"))
			editor.AddTrigger();
		y += EditorUI::ROW_HEIGHT + 4.0f;
		GameActor* selected = editor.GetSelected();
		GuiSetState(selected ? STATE_NORMAL : STATE_DISABLED);
		if (GuiButton({ x, y, half, EditorUI::ROW_HEIGHT }, "Delete"))
			editor.DeleteSelected();
		if (GuiButton({ x + half + EditorUI::PADDING, y, half, EditorUI::ROW_HEIGHT }, "Duplicate"))
			editor.DuplicateSelected();
		GuiSetState(STATE_NORMAL);
		y += EditorUI::ROW_HEIGHT + 4.0f;
		if (GuiButton({ x, y, w, EditorUI::ROW_HEIGHT }, "Set Spawn Here (camera)"))
			editor.SetSpawnHere();
		y += EditorUI::ROW_HEIGHT + 8.0f;

		// Rename box for the selection
		selected = editor.GetSelected();
		if (selected != m_nameOwner)
		{
			m_isRenaming = false;
			m_nameOwner = selected;
			std::strncpy(m_nameBuffer, selected ? selected->GetName().c_str() : "", sizeof(m_nameBuffer) - 1);
			m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
		}
		GuiSetState(selected ? STATE_NORMAL : STATE_DISABLED);
		if (GuiTextBox({ x, y, w, EditorUI::ROW_HEIGHT }, m_nameBuffer, sizeof(m_nameBuffer), m_isRenaming))
			m_isRenaming = !m_isRenaming;
		GuiSetState(STATE_NORMAL);
		if (selected && m_isRenaming && m_nameBuffer[0] != '\0')
			selected->SetName(m_nameBuffer);
		y += EditorUI::ROW_HEIGHT + 8.0f;

		// Tree
		const Rectangle panel = { area.x + 2.0f, y, area.width - 4.0f, area.y + area.height - y - 2.0f };
		const float contentHeight = static_cast<float>(level.GetActorCount()) * EditorUI::ROW_HEIGHT;
		const Rectangle content = { 0.0f, 0.0f, panel.width - 16.0f, contentHeight };
		Rectangle view = {};
		GuiScrollPanel(panel, nullptr, content, &m_scroll, &view);

		BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width), static_cast<int>(view.height));
		float rowY = view.y + m_scroll.y;
		for (const auto& owned : level.GetActors())
			if (!owned->GetParent())
				DrawRows(editor, level, owned.get(), 0, rowY, view);
		EndScissorMode();
	}

	void HierarchyPanel::DrawRows(Editor& editor, const Level& level, GameActor* actor, int depth, float& y, const Rectangle& view)
	{
		if (actor->IsMarkedForDestroy())
			return;

		const Rectangle row = { view.x, y, view.width, EditorUI::ROW_HEIGHT };
		const bool isSelected = actor == editor.GetSelected();
		const bool isVisible = row.y + row.height >= view.y && row.y <= view.y + view.height;

		if (isVisible)
		{
			const bool isHovered = CheckCollisionPointRec(GetMousePosition(), row) && CheckCollisionPointRec(GetMousePosition(), view);
			if (isSelected)			DrawRectangleRec(row, Fade(ORANGE, 0.45f));
			else if (isHovered)		DrawRectangleRec(row, Fade(SKYBLUE, 0.35f));

			const Color color = actor->IsActiveInHierarchy() ? GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)) : GRAY;
			DrawText(actor->GetName().c_str(), static_cast<int>(row.x + 6.0f + 14.0f * depth), static_cast<int>(row.y + 5.0f),
				EditorUI::TEXT_SIZE, color);

			if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				editor.Select(actor);
		}
		y += EditorUI::ROW_HEIGHT;

		for (GameActor* child : actor->GetChildren())
			DrawRows(editor, level, child, depth + 1, y, view);
	}
}
