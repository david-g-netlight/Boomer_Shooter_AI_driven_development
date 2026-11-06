#pragma once

namespace eng
{
	/// <summary>
	/// Engine name and version, used for the window title and logs.
	/// </summary>
	const char* GetEngineName();
	const char* GetEngineVersion();

	/// <summary>
	/// True when the editor is compiled in (ENG_EDITOR=1).
	/// </summary>
	bool IsEditorBuild();
}
