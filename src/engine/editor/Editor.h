#pragma once
#include "editor/LevelSnapshot.h"

namespace eng
{
	class Level;

	/// <summary>
	/// Editor-mode coordinator, owned by the Engine and used only in editor builds (ENG_EDITOR=1).
	/// The ModeMachine calls Enter() / Exit() on the Play <-> Editor transitions (TAB). More tools
	/// (camera, picker, gizmo, panels) are added by the next Phase 9 tasks.
	/// </summary>
	class Editor
	{
	public:
		/// <summary>
		/// Captures a fresh snapshot of `level` (every entry from Play takes one).
		/// </summary>
		void				Enter(Level* level);

		/// <summary>
		/// Leaves the editor with the edits live. The snapshot stays for Reset() until the next Enter().
		/// </summary>
		void				Exit();

		/// <summary>
		/// Restores the level to the state it had when the editor was entered.
		/// </summary>
		void				Reset();

		bool				IsActive() const;
		Level*				GetLevel() const;
		const LevelSnapshot& GetSnapshot() const;

	private:
		Level*			m_level		= nullptr;
		bool			m_isActive	= false;
		LevelSnapshot	m_snapshot;
	};
}
