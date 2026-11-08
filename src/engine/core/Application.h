#pragma once
#include <string>
#include "modes/EngineMode.h"

namespace eng
{
	class Level; // Phase 5
	class RenderQueue;

	/// <summary>
	/// Interface between the Engine and the game.
	/// The Engine owns the Application and is the only class allowed to delete it.
	/// </summary>
	class Application
	{
	public:
		virtual ~Application() = default;

		/// <summary>
		/// Called once after the window exists. Separates "object exists" from "object ready for use".
		/// Returning false aborts the start-up; Destroy() is then NOT called.
		/// </summary>
		virtual bool Init() = 0;

		/// <summary>
		/// Called once per frame with the clamped, scaled delta time.
		/// </summary>
		virtual void Update(float deltaTime) = 0;

		/// <summary>
		/// Called 0..N times per frame at a fixed rate (eng::FIXED_DELTA_TIME). Physics-related logic goes here.
		/// </summary>
		virtual void FixedUpdate(float /*fixedDeltaTime*/) {}

		/// <summary>
		/// Called every frame during rendering (also while paused): submit what should be drawn this frame.
		/// The Engine draws the queue with the active camera, then clears it.
		/// </summary>
		virtual void SubmitRenderables(RenderQueue& /*queue*/) {}

		/// <summary>
		/// 2D drawing after the 3D pass (HUD, crosshair, debug text).
		/// </summary>
		virtual void DrawHUD() {}

		/// <summary>
		/// Logical clean-up, called by Engine::Shutdown() while the window still exists.
		/// </summary>
		virtual void Destroy() = 0;

		//////////////////////////////////////////////////////////////
		/// Mode hooks, called by the ModeMachine (Phase 8)
		//////////////////////////////////////////////////////////////
		virtual void OnStartRequested()		{} // Main menu "Start"
		virtual void OnPause()				{}
		virtual void OnResume()				{}
		virtual void OnEnterEditor()		{}
		virtual void OnExitEditor()			{}
		virtual void OnReturnToMainMenu()	{}
		virtual void OnRestartRequested()	{}

		/// <summary>
		/// The editor cleared the active level and loaded a file into it: every pointer to an old actor is gone.
		/// </summary>
		virtual void OnLevelLoaded()		{}

		/// <summary>
		/// Where the editor saves and loads the active level, relative to the executable ("assets/levels/x.lvl").
		/// Empty = no Save / Load.
		/// </summary>
		virtual std::string GetLevelPath() const { return {}; }

		/// <summary>
		/// Mode the engine enters after Init(). Defaults to the main menu.
		/// </summary>
		virtual EngineMode	GetStartMode() const { return EngineMode::MainMenu; }

		/// <summary>
		/// Window title. Defaults to the engine name.
		/// </summary>
		virtual const char*	GetTitle() const;

		/// <summary>
		/// Level used by the renderer and the editor (Phase 5). Non-owning.
		/// </summary>
		virtual Level*		GetActiveLevel() { return nullptr; }

				void SetNeedsToBeClosed(bool value);
				bool NeedsToBeClosed() const;

	private:
		bool m_needsToBeClosed = false;
	};
}
