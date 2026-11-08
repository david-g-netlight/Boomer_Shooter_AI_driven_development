#pragma once
#include <cstdint>

namespace eng
{
	/// <summary>
	/// What the whole engine is doing. Only Play runs the simulation (fixed step, physics, Update).
	/// </summary>
	enum class EngineMode : uint8_t
	{
		MainMenu,
		Play,
		Paused,
		Editor,		// editor builds only (ENG_EDITOR=1)
	};

	constexpr const char* ModeName(EngineMode mode)
	{
		switch (mode)
		{
			case EngineMode::MainMenu:	return "MainMenu";
			case EngineMode::Play:		return "Play";
			case EngineMode::Paused:	return "Paused";
			case EngineMode::Editor:	return "Editor";
		}
		return "?";
	}

	/// <summary>
	/// Published on the EventBus after every mode change.
	/// </summary>
	struct ModeChangedEvent
	{
		EngineMode	previous;
		EngineMode	current;
		bool		isRestart	= false;
	};
}
