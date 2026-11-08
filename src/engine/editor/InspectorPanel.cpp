#include "InspectorPanel.h"
#include <algorithm>
#include <raymath.h>
#include "editor/ActorEdit.h"
#include "editor/Editor.h"
#include "editor/EditorUI.h"
#include "level/Component.h"
#include "level/GameActor.h"

namespace eng
{
	bool InspectorPanel::IsEditingText() const { return m_layout.IsEditing(); }

	void InspectorPanel::Draw(Editor& editor)
	{
		EditorUI::TextScope text;
		const Rectangle area = EditorUI::InspectorRect();
		GuiPanel(area, "Inspector");

		GameActor* actor = editor.GetSelected();
		if (actor != m_shown)
		{
			m_shown = actor;
			m_layout.CancelEditing();
			m_scroll = { 0.0f, 0.0f };
		}

		const Rectangle panel = { area.x + 2.0f, area.y + 26.0f, area.width - 4.0f, area.height - 28.0f };
		if (!actor)
		{
			GuiLabel({ panel.x + EditorUI::PADDING, panel.y + 4.0f, panel.width, EditorUI::ROW_HEIGHT }, "Nothing selected");
			return;
		}

		Rectangle view = {};
		GuiScrollPanel(panel, nullptr, { 0.0f, 0.0f, panel.width - 16.0f, m_contentHeight }, &m_scroll, &view);
		BeginScissorMode(static_cast<int>(view.x), static_cast<int>(view.y), static_cast<int>(view.width), static_cast<int>(view.height));

		m_layout.Begin(view, m_scroll.y);

		bool isActive = actor->IsActive();
		if (m_layout.Bool(actor->GetName().c_str(), isActive))
			actor->SetActive(isActive);

		// Transform: local space, rotation as euler degrees
		m_layout.Header("Transform");
		::Transform transform = actor->GetTransform();
		const Vector3 radians = QuaternionToEuler(transform.rotation);
		Vector3 euler = { radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG };

		bool changed = false;
		changed |= m_layout.Vec3("Position", transform.translation);
		if (m_layout.Vec3("Rotation (deg)", euler))
		{
			transform.rotation = QuaternionFromEuler(euler.x * DEG2RAD, euler.y * DEG2RAD, euler.z * DEG2RAD);
			changed = true;
		}
		if (m_layout.Vec3("Scale", transform.scale))
		{
			transform.scale = { std::max(transform.scale.x, 0.01f), std::max(transform.scale.y, 0.01f), std::max(transform.scale.z, 0.01f) };
			changed = true;
		}
		if (changed)
			ActorEdit::SetLocalTransform(*actor, transform);

		// Components
		for (const auto& component : actor->GetComponents())
		{
			m_layout.Header(component->TypeName());
			bool isEnabled = component->IsEnabled();
			if (m_layout.Bool("Enabled", isEnabled))
				component->SetEnabled(isEnabled);
			component->DrawInspector(m_layout);
		}

		m_contentHeight = m_layout.End() + 8.0f;
		EndScissorMode();
	}
}
