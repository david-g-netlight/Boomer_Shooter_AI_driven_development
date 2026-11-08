#include "Editor.h"
#include <cmath>
#include "core/Engine.h"
#include "core/Log.h"
#include "input/InputManager.h"
#include "level/GameActor.h"
#include "level/Level.h"
#include "editor/Picker.h"

namespace eng
{
	void Editor::Enter(Level* level)
	{
		m_level		= level;
		m_isActive	= true;

		// Fly from where the player was looking
		ActiveCamera& active = Engine::GetInstance().GetMainCamera();
		m_previousCamera = active.GetActiveCamera();
		if (m_previousCamera)
			m_camera.CopyPoseFrom(*m_previousCamera);
		active.SetActiveCamera(m_camera.GetCamera());

		if (!m_level)
		{
			m_snapshot.Clear();
			Log::Warning("Editor: no active level, nothing to edit");
			return;
		}

		m_selected = nullptr;
		m_snapshot.Capture(*m_level);
		Log::Info("Editor: entered, snapshot of %zu actors", m_snapshot.GetActorCount());
	}

	void Editor::Exit()
	{
		m_isActive = false;
		m_selected = nullptr;
		if (m_previousCamera)
			Engine::GetInstance().GetMainCamera().SetActiveCamera(*m_previousCamera);
		m_previousCamera = nullptr;
		Log::Info("Editor: exited, edits stay live");
	}

	void Editor::Reset()
	{
		if (!m_level || m_snapshot.IsEmpty())
			return;

		const LevelSnapshot::RestoreResult result = m_snapshot.Restore(*m_level);
		Log::Info("Editor: reset, %d restored, %d new actors removed", result.restored, result.removed);
		if (result.missing > 0)
			Log::Warning("Editor: %d deleted actors cannot be restored yet", result.missing);
	}

	void Editor::Update(InputManager& input, float unscaledDeltaTime)
	{
		if (!m_isActive)
			return;

		ValidateSelection();
		m_camera.Update(input, unscaledDeltaTime);

		const bool isLooking = input.IsCursorLocked();	// RMB fly mode: clicks belong to the camera
		if (m_level && !isLooking && input.IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			Select(Picker::Pick(m_camera.GetCamera(), input.GetMousePosition(), *m_level, Engine::GetInstance().GetPhysics()));

		if (m_selected && input.IsKeyPressed(KEY_F))
		{
			const BoundingBox box = Picker::GetActorBounds(*m_selected);
			const Vector3 center	= { (box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f, (box.min.z + box.max.z) * 0.5f };
			const Vector3 extent	= { box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z };
			m_camera.FocusOn(center, 0.5f * std::sqrt(extent.x * extent.x + extent.y * extent.y + extent.z * extent.z));
		}
	}

	void Editor::ValidateSelection()
	{
		if (!m_selected)
			return;

		// The Level deletes marked actors at the end of the frame: a pointer that is no longer in it is gone
		bool found = false;
		if (m_level && !m_selected->IsMarkedForDestroy())
			for (const auto& owned : m_level->GetActors())
				if (owned.get() == m_selected) { found = true; break; }
		if (!found)
			m_selected = nullptr;
	}

	void Editor::Select(GameActor* actor)
	{
		m_selected = actor;
		if (actor)
			Log::Info("Editor: selected '%s'", actor->GetName().c_str());
	}

	GameActor* Editor::GetSelected() const	{ return m_selected; }

	void Editor::Draw3D() const
	{
		if (!m_isActive || !m_selected || m_selected->IsMarkedForDestroy())
			return;

		DrawBoundingBox(Picker::GetActorBounds(*m_selected), ORANGE);
	}

	EditorCamera& Editor::GetCamera()					{ return m_camera; }
	bool Editor::IsActive() const						{ return m_isActive; }
	Level* Editor::GetLevel() const						{ return m_level; }
	const LevelSnapshot& Editor::GetSnapshot() const	{ return m_snapshot; }
}
