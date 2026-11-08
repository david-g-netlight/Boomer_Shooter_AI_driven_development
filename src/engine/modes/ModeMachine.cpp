#include "ModeMachine.h"
#include "core/Application.h"
#include "core/Engine.h"
#include "core/EngineInfo.h"
#include "core/Log.h"
#include "input/InputManager.h"
#include "ui/MenuScreens.h"
#include "ui/UIStack.h"

namespace eng
{
	EngineMode ModeMachine::GetMode() const		{ return m_mode; }
	bool ModeMachine::IsSimulating() const		{ return m_mode == EngineMode::Play; }
	bool ModeMachine::HasPendingRequest() const	{ return m_pending.has_value(); }

	bool ModeMachine::CanTransition(EngineMode from, EngineMode to) const
	{
		if (from == to)
			return false;
		if (to == EngineMode::Editor || from == EngineMode::Editor)
			if (!IsEditorBuild())
				return false;

		switch (from)
		{
			case EngineMode::MainMenu:	return to == EngineMode::Play || to == EngineMode::Editor;
			case EngineMode::Play:		return true;	// Paused, Editor, MainMenu
			case EngineMode::Paused:	return to == EngineMode::Play || to == EngineMode::MainMenu;
			case EngineMode::Editor:	return to == EngineMode::Play || to == EngineMode::MainMenu;
		}
		return false;
	}

	bool ModeMachine::Request(EngineMode target)
	{
		const EngineMode from = m_pending.value_or(m_mode);
		if (!CanTransition(from, target))
		{
			Log::Warning("ModeMachine: %s -> %s is not allowed", ModeName(from), ModeName(target));
			return false;
		}
		m_pending	= target;
		m_isRestart	= false;
		return true;
	}

	bool ModeMachine::RequestRestart()
	{
		if (m_mode != EngineMode::Play && m_mode != EngineMode::Paused)
		{
			Log::Warning("ModeMachine: restart is only allowed from Play or Paused");
			return false;
		}
		m_pending	= EngineMode::Play;
		m_isRestart	= true;
		return true;
	}

	void ModeMachine::Begin(EngineMode initial)
	{
		m_mode = initial;
		m_pending.reset();
		ApplyModeEffects(initial);
		Engine::GetInstance().GetEvents().Publish(ModeChangedEvent{ initial, initial, false });
		Log::Info("ModeMachine: starting in %s", ModeName(initial));
	}

	void ModeMachine::HandleInput(const InputManager& input)
	{
		if (input.IsKeyPressed(KEY_ESCAPE))
		{
			if (m_mode == EngineMode::Play)		Request(EngineMode::Paused);
			else if (m_mode == EngineMode::Paused)	Request(EngineMode::Play);
		}

		if (IsEditorBuild() && input.IsKeyPressed(KEY_TAB))
		{
			if (m_mode == EngineMode::Play)		Request(EngineMode::Editor);
			else if (m_mode == EngineMode::Editor)	Request(EngineMode::Play);
		}
	}

	void ModeMachine::HandleFocus(bool isFocused)
	{
		// Only the edge pauses: coming back to the window never resumes by itself
		const bool lostFocus = m_wasFocused && !isFocused;
		m_wasFocused = isFocused;

		if (lostFocus && m_mode == EngineMode::Play && !m_pending)
		{
			Log::Info("ModeMachine: window lost focus, pausing");
			Request(EngineMode::Paused);
		}
	}

	void ModeMachine::ApplyPendingTransition()
	{
		if (!m_pending)
			return;

		const EngineMode from		= m_mode;
		const EngineMode to			= *m_pending;
		const bool		 isRestart	= m_isRestart;
		m_pending.reset();
		m_isRestart = false;

		if (from == to && !isRestart)
			return;

		Application* app = Engine::GetInstance().GetApplication();
		m_mode = to;

		// 1. Application hooks
		if (app)
		{
			if (from == EngineMode::Editor)		app->OnExitEditor();

			if (isRestart)										app->OnRestartRequested();
			else if (to == EngineMode::Play && from == EngineMode::MainMenu)	app->OnStartRequested();
			else if (to == EngineMode::Play && from == EngineMode::Paused)	app->OnResume();
			else if (to == EngineMode::Paused)					app->OnPause();
			else if (to == EngineMode::MainMenu)				app->OnReturnToMainMenu();

			if (to == EngineMode::Editor)		app->OnEnterEditor();
		}

		// 2 + 3 + 4. Cursor, audio, mode screen
		ApplyModeEffects(to);

		// 5. Event
		Engine::GetInstance().GetEvents().Publish(ModeChangedEvent{ from, to, isRestart });
		Log::Info("ModeMachine: %s -> %s%s", ModeName(from), ModeName(to), isRestart ? " (restart)" : "");
	}

	void ModeMachine::ApplyModeEffects(EngineMode mode)
	{
		Engine&			engine	= Engine::GetInstance();
		AudioManager&	audio	= engine.GetAudio();
		UIStack&		ui		= engine.GetUI();

		// Replace the previous mode's screen (game screens such as dialogs are left alone)
		ui.Remove(m_modeScreen);
		m_modeScreen = nullptr;
		switch (mode)
		{
			case EngineMode::MainMenu:	m_modeScreen = ui.Push<MainMenuScreen>();		break;
			case EngineMode::Paused:	m_modeScreen = ui.Push<PauseMenuScreen>();		break;
			case EngineMode::Editor:	m_modeScreen = ui.Push<EditorBannerScreen>();	break;	// TODO(Phase 9)
			case EngineMode::Play:		break;
		}

		engine.GetInputManager().SetCursorLocked(mode == EngineMode::Play);

		const bool quiet = mode == EngineMode::Paused || mode == EngineMode::Editor;
		audio.Duck(quiet);
		if (quiet)
			audio.PauseAll();
		else
			audio.ResumeAll();
	}
}
