#pragma once
#include "editor/EditorCamera.h"
#include "editor/HierarchyPanel.h"
#include "editor/LevelSnapshot.h"
#include "editor/TransformGizmo.h"

namespace eng
{
	class GameActor;
	class Level;
	class InputManager;


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

		/// <summary>
		/// Once per frame while the editor is active (unscaled time).
		/// </summary>
		void				Update(InputManager& input, float unscaledDeltaTime);

		/// <summary>
		/// Selection highlight. Call inside the 3D pass (BeginMode3D / EndMode3D) while the editor is active.
		/// </summary>
		void				Draw3D() const;

		//////////////////////////////////////////
		/// Selection (one actor; never dangles: dropped as soon as the actor is marked or gone)
		//////////////////////////////////////////
		void				Select(GameActor* actor);
		GameActor*			GetSelected() const;

		/// <summary>
		/// Editor panels (raygui, 2D). Call in the HUD pass while the editor is active.
		/// </summary>
		void				DrawPanels();

		//////////////////////////////////////////
		/// Editing commands (hierarchy buttons, hotkeys)
		//////////////////////////////////////////
		GameActor*			AddCube();			// dynamic prop 4 m in front of the editor camera
		GameActor*			AddTrigger();		// translucent trigger volume 4 m in front of the editor camera
		void				DeleteSelected();

		/// <summary>
		/// Shader given to new cubes (the game's lit shader). nullptr = raylib's default.
		/// </summary>
		void				SetDefaultShader(const Shader* shader);

		/// <summary>
		/// True while a text box has the keyboard: hotkeys stay quiet.
		/// </summary>
		bool				IsTyping() const;

		bool				IsActive() const;
		EditorCamera&		GetCamera();
		TransformGizmo&		GetGizmo();
		Level*				GetLevel() const;
		const LevelSnapshot& GetSnapshot() const;

	private:
		void				ValidateSelection();

		Level*			m_level		= nullptr;
		GameActor*		m_selected	= nullptr;
		Camera*			m_previousCamera = nullptr;	// the game's camera, active again after Exit()
		EditorCamera	m_camera;
		TransformGizmo	m_gizmo;
		HierarchyPanel	m_hierarchy;
		const Shader*	m_defaultShader	= nullptr;
		int				m_spawnCounter	= 0;
		bool			m_isActive	= false;
		LevelSnapshot	m_snapshot;
	};
}
