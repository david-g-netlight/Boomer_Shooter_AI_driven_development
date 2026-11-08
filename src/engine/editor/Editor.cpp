#include "Editor.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "level/Level.h"

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

		m_snapshot.Capture(*m_level);
		Log::Info("Editor: entered, snapshot of %zu actors", m_snapshot.GetActorCount());
	}

	void Editor::Exit()
	{
		m_isActive = false;
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
		if (m_isActive)
			m_camera.Update(input, unscaledDeltaTime);
	}

	EditorCamera& Editor::GetCamera()					{ return m_camera; }
	bool Editor::IsActive() const						{ return m_isActive; }
	Level* Editor::GetLevel() const						{ return m_level; }
	const LevelSnapshot& Editor::GetSnapshot() const	{ return m_snapshot; }
}
