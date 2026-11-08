#pragma once

namespace eng
{
	class Editor;

	/// <summary>
	/// Strip between the two panels: Play (leave the editor, edits stay live), Reset (back to the state at entry),
	/// Move / Scale gizmo mode, Save and Load (Ctrl+S saves). Immediate mode: call Draw() every frame in the HUD pass.
	/// </summary>
	class EditorToolbar
	{
	public:
		void	Draw(Editor& editor);
	};
}
