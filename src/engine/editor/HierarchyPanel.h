#pragma once
#include <raylib.h>

namespace eng
{
	class Editor;
	class GameActor;
	class Level;

	/// <summary>
	/// Left panel: the actor tree (select), rename box for the selection and the Add Cube / Add Trigger /
	/// Delete buttons. Immediate mode: call Draw() every frame in the HUD pass.
	/// </summary>
	class HierarchyPanel
	{
	public:
		void	Draw(Editor& editor, Level& level);

		/// <summary>
		/// True while the rename box has the keyboard (editor hotkeys must stay quiet).
		/// </summary>
		bool	IsEditingText() const;

	private:
		void	DrawRows(Editor& editor, const Level& level, GameActor* actor, int depth, float& y, const Rectangle& view);

		Vector2	m_scroll		= { 0.0f, 0.0f };
		bool	m_isRenaming	= false;
		char	m_nameBuffer[64]= {};
		const GameActor* m_nameOwner = nullptr;	// whose name the buffer holds
	};
}
