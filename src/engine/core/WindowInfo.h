#pragma once

namespace eng
{
	/// <summary>
	/// Window sizes, refreshed by the Engine every frame.
	/// screen* = logical points (UI, mouse), render* = framebuffer pixels (render targets).
	/// They differ on HighDPI (Retina) displays.
	/// </summary>
	struct WindowInfo
	{
		int		screenWidth		= 0;
		int		screenHeight	= 0;
		int		renderWidth		= 0;
		int		renderHeight	= 0;
		float	dpiScale		= 1.0f;
		bool	wasResized		= false;	// true only on the frame the window changed size
		bool	isFocused		= true;

		/// <summary>
		/// Reads the current state from raylib. Call once per frame, after the window exists.
		/// </summary>
		void Refresh();
	};
}
