#include "WindowInfo.h"
#include <raylib.h>

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
}
