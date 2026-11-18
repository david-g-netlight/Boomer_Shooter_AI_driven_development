#include "EngineInfo.h"

namespace eng
{
	const char* GetEngineName()
	{
		return "Raylib Engine";
	}

	const char* GetEngineVersion()
	{
		return "0.0.1";
	}

	bool IsEditorBuild()
	{
		return ENG_EDITOR == 1;
	}

	bool IsDevConsoleBuild()
	{
		return ENG_DEV_CONSOLE == 1;
	}
}
