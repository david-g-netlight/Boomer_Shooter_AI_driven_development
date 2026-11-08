#include "Editor.h"
#include "core/Log.h"
#include "level/Level.h"

namespace eng
{
	void Editor::Enter(Level* level)
	{
		m_level		= level;
		m_isActive	= true;

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

	bool Editor::IsActive() const						{ return m_isActive; }
	Level* Editor::GetLevel() const						{ return m_level; }
	const LevelSnapshot& Editor::GetSnapshot() const	{ return m_snapshot; }
}
