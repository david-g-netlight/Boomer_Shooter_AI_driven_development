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

	/// <summary>
	/// Switches borderless fullscreen on or off (raylib's ToggleBorderlessWindowed) and keeps raylib's screen size
	/// right. On macOS with HighDPI, raylib divides the new size by the DPI scale once too often (a 1512 x 982 screen
	/// reads 756 x 491, and 640 x 360 after switching back), so the 2D pass (HUD, menus) covers a quarter of the
	/// window; the size is set again here: the monitor's in borderless, the window's from before when leaving it.
	/// </summary>
	void SetBorderlessWindowed(bool enabled);
}
