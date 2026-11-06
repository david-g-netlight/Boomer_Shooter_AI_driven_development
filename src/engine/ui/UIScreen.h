#pragma once

namespace eng
{
	/// <summary>
	/// One screen of UI (menu, dialog, HUD overlay), drawn with raygui in screen coordinates (points).
	/// Lives on the UIStack; only the top screen is interactive. Buttons react inside Draw() (raygui is
	/// immediate mode); pushing / removing screens from there is safe (the stack defers the change).
	/// </summary>
	class UIScreen
	{
	public:
		virtual ~UIScreen() = default;

		virtual void		OnPush() {}
		virtual void		OnPop() {}
		virtual void		Update(float /*unscaledDeltaTime*/) {}
		virtual void		Draw() = 0;

		/// <summary>
		/// Opaque screens hide everything below them on the stack (the 3D scene is still drawn).
		/// </summary>
		virtual bool		IsOpaque() const { return false; }

		virtual const char*	GetName() const = 0;
	};
}
