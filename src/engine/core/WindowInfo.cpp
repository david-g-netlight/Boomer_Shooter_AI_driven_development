#include "WindowInfo.h"
#include <raylib.h>
#include "core/Log.h"

namespace eng
{
	void WindowInfo::Refresh()
	{
		screenWidth		= GetScreenWidth();
		screenHeight	= GetScreenHeight();
		renderWidth		= GetRenderWidth();
		renderHeight	= GetRenderHeight();
		dpiScale		= GetWindowScaleDPI().x;
		wasResized		= IsWindowResized();
		isFocused		= IsWindowFocused();
	}

	void SetBorderlessWindowed(bool enabled)
	{
		if (!IsWindowReady() || enabled == IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))
			return;

		static int windowedWidth	= 0;	// the windowed size to go back to (raylib's own copy is halved, see above)
		static int windowedHeight	= 0;
		const int monitor = GetCurrentMonitor();
		if (enabled)
		{
			windowedWidth	= GetScreenWidth();
			windowedHeight	= GetScreenHeight();
		}

		ToggleBorderlessWindowed();

#ifdef __APPLE__
		if (enabled)
			SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
		else if (windowedWidth > 0 && windowedHeight > 0)
			SetWindowSize(windowedWidth, windowedHeight);
#endif

		Log::Info("Window: %s, %d x %d", enabled ? "borderless fullscreen" : "windowed", GetScreenWidth(), GetScreenHeight());
	}
}
