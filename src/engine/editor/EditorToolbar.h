#pragma once

namespace eng
{
	class Editor;

	/// <summary>
	/// Strip under the status line: Play (leave the editor, edits stay live), Reset (back to the state at entry),
	/// Move / Scale gizmo mode, Physics debug, Save (Ctrl+S saves). Load is console only (`load`).
	/// Immediate mode: call Draw() every frame in the HUD pass.
	/// </summary>
	class EditorToolbar
	{
	public:
		void	Draw(Editor& editor);
	};
}
