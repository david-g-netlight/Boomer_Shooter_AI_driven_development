#pragma once
#include <raylib.h>
#include "editor/InspectorLayout.h"

namespace eng
{
	class Editor;
	class GameActor;

	/// <summary>
	/// Right panel: the selected actor's active flag, transform (position, euler degrees, scale) and one
	/// section per component, filled by Component::DrawInspector(). Edits go through ActorEdit, so physics
	/// bodies follow. Immediate mode: call Draw() every frame in the HUD pass.
	/// </summary>
	class InspectorPanel
	{
	public:
		void	Draw(Editor& editor);

		/// <summary>
		/// True while a number field has the keyboard (editor hotkeys must stay quiet).
		/// </summary>
		bool	IsEditingText() const;

	private:
		InspectorLayout		m_layout;
		Vector2				m_scroll			= { 0.0f, 0.0f };
		float				m_contentHeight		= 0.0f;		// measured last frame
		const GameActor*	m_shown				= nullptr;
	};
}
