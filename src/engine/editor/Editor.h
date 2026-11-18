#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "editor/EditorCamera.h"
#include "editor/EditorToolbar.h"
#include "editor/LevelSnapshot.h"
#include "editor/TransformGizmo.h"

namespace eng
{
	class Component;
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
		GameActor*			GetSelected() const;	// nullptr when the selected actor is gone (level reloaded, deleted)

		/// <summary>
		/// The level the editor works on: its own while active, otherwise the Application's active level (the
		/// console's ls / select / pick / info also work in Play).
		/// </summary>
		Level*				ResolveLevel() const;

		/// <summary>
		/// What is under the middle of the screen, seen from the active camera (works in any mode). Ground, Prop
		/// and Trigger layers only, so the player's own capsule is never picked. nullptr when nothing is hit.
		/// </summary>
		GameActor*			PickAtScreenCenter() const;

		/// <summary>
		/// Editor camera to the selection (F).
		/// </summary>
		void				FocusSelected();

		/// <summary>
		/// Editor panels (raygui, 2D). Call in the HUD pass while the editor is active.
		/// </summary>
		void				DrawPanels();

		//////////////////////////////////////////
		/// Editing commands (hotkeys, console)
		//////////////////////////////////////////
		GameActor*			AddCube();			// dynamic prop 4 m in front of the editor camera
		GameActor*			AddEmpty();			// bare actor 4 m in front of the editor camera
		GameActor*			AddTrigger();		// translucent trigger volume 4 m in front of the editor camera
		void				DeleteSelected();

		/// <summary>
		/// The level's spawn point goes where the editor camera is: on the ground below it (a physics ray down),
		/// facing the way the camera looks.
		/// </summary>
		void				SetSpawnHere();

		/// <summary>
		/// Components by registered type name (what the inspector's fields and Add Component did).
		/// SetComponentFields: every key must already be written by the component's Serialize(); nothing changes
		/// when one is unknown. The component is read back through Deserialize(), then OnFieldsChanged().
		/// RemoveComponent refuses a Collider while a RigidBody / TriggerVolume / CharacterMotor still needs it.
		/// Failures return false / nullptr and say why in `error`.
		/// </summary>
		bool				SetComponentFields(GameActor& actor, const std::string& type,
												const std::vector<std::pair<std::string, std::string>>& fields, std::string& error);
		Component*			AddComponent(GameActor& actor, const std::string& type, std::string& error);
		bool				RemoveComponent(GameActor& actor, const std::string& type, std::string& error);

		/// <summary>
		/// Copies the selection (with children) next to it and selects the copy. Needs every component registered.
		/// </summary>
		GameActor*			DuplicateSelected();

		/// <summary>
		/// Level file of the Application (Application::GetLevelPath). Save writes it (and, in editor builds, the
		/// copy in the source tree so the build's asset copy can't overwrite it). Load is applied at the start of
		/// the next editor frame: the level is cleared, the file loaded, Application::OnLevelLoaded() called,
		/// and the loaded state becomes the new Reset point.
		/// </summary>
		bool				CanSaveLoad() const;
		/// <summary>
		/// An empty path means the Application's level file; an explicit path is written / read as given.
		/// </summary>
		void				Save(const std::string& path = {});
		void				RequestLoad(const std::string& path = {});

		/// <summary>
		/// Called on every prop the editor creates (Add Cube): the game adds its own components (none by default).
		/// </summary>
		void				SetPropDecorator(std::function<void(GameActor&)> decorator);

		bool				IsActive() const;
		EditorCamera&		GetCamera();
		TransformGizmo&		GetGizmo();
		Level*				GetLevel() const;
		const LevelSnapshot& GetSnapshot() const;

	private:
		void				ValidateSelection();
		void				DrawStatus();
		void				LoadNow();
		std::string			GetLevelPath() const;

		Level*			m_level		= nullptr;
		GameActor*		m_selected	= nullptr;
		Camera*			m_previousCamera = nullptr;	// the game's camera, active again after Exit()
		EditorCamera	m_camera;
		TransformGizmo	m_gizmo;
		bool			m_physicsDebugWasOn = false;	// restored on Exit()
		EditorToolbar	m_toolbar;
		bool			m_isLoadPending	= false;
		std::string		m_loadPath;					// empty = the Application's level file
		std::function<void(GameActor&)>	m_propDecorator;
		int				m_spawnCounter	= 0;
		bool			m_isActive	= false;
		LevelSnapshot	m_snapshot;
	};
}
