#pragma once
#if ENG_DEV_CONSOLE && ENG_EDITOR

namespace eng
{
	class Engine;
	class DevConsole;

	/// <summary>
	/// Everything the editor panels used to do, one command each: editor / play, ls, select, pick, deselect, info,
	/// rename, pos, move, rot, scale, active, parent, add, dup, del, focus, spawn_here, spawn, reset, save, load,
	/// plus the component commands (set, get, comp_add, comp_rm, comp_types). Called by Engine::Init.
	/// </summary>
	void RegisterEditorCommands(Engine& engine, DevConsole& console);
}

#endif
