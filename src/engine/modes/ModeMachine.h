#pragma once
#include <optional>
#include "modes/EngineMode.h"

namespace eng
{
	class InputManager;
	class UIScreen;

	/// <summary>
	/// Engine modes and their transitions, owned by the Engine.
	///
	///   MainMenu -> Play (Start)            Play -> Paused (ESC, focus lost)    Paused -> Play (ESC, Resume)
	///   Paused / Play / Editor -> MainMenu  Play <-> Editor (TAB, editor builds) MainMenu -> Editor (editor builds)
	///
	/// Request() is validated immediately and applied at the start of the next frame, so a menu button can
	/// request a change while its own screen is being drawn. On every applied transition:
	///   1. the Application hook (OnStartRequested, OnPause, OnResume, OnEnterEditor, OnExitEditor,
	///      OnReturnToMainMenu, OnRestartRequested)
	///   2. cursor: locked in Play, free otherwise
	///   3. audio: Paused / Editor duck the music and pause sound effects; Play / MainMenu restore them
	///   4. the mode's screen (MainMenuScreen, PauseMenuScreen, editor banner) replaces the previous one
	///   5. ModeChangedEvent on the EventBus
	/// </summary>
	class ModeMachine
	{
	public:
		EngineMode	GetMode() const;
		bool		IsSimulating() const;		// true only in Play

		bool		CanTransition(EngineMode from, EngineMode to) const;

		/// <summary>
		/// Returns false (and logs) if the transition isn't allowed from the current mode.
		/// </summary>
		bool		Request(EngineMode target);

		/// <summary>
		/// Application::OnRestartRequested(), then Play. Allowed from Play and Paused.
		/// </summary>
		bool		RequestRestart();

		bool		HasPendingRequest() const;

		//////////////////////////////////////////
		/// Called by the Engine
		//////////////////////////////////////////
		void		Begin(EngineMode initial);				// after Application::Init
		void		HandleInput(const InputManager& input);	// ESC / TAB
		void		HandleFocus(bool isFocused);			// Play -> Paused on the frame the window loses focus
		void		ApplyPendingTransition();

	private:
		void		ApplyModeEffects(EngineMode mode);

		EngineMode					m_mode			= EngineMode::MainMenu;
		std::optional<EngineMode>	m_pending;
		bool						m_isRestart		= false;
		bool						m_wasFocused	= true;
		const UIScreen*				m_modeScreen	= nullptr;	// the screen this machine pushed for the current mode
	};
}
