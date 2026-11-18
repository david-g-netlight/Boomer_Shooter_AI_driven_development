#pragma once
#if ENG_DEV_CONSOLE

namespace eng
{
	class Engine;
	class DevConsole;

	/// <summary>
	/// Registers the engine's own commands: help, clear, echo, quit, mode, timescale, con_pause,
	/// phys_debug, grid, fps, exec. Called by Engine::Init, before the game registers its commands.
	/// </summary>
	void RegisterEngineCommands(Engine& engine, DevConsole& console);
}

#endif
