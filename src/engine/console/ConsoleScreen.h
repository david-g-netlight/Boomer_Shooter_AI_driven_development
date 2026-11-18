#pragma once
#if ENG_DEV_CONSOLE

#include "ui/UIScreen.h"

namespace eng
{
	class DevConsole;

	/// <summary>
	/// The console's drop-down panel, on the UIStack while the console is open. Not opaque: the game shows below.
	/// Frees the cursor on push and restores the lock on pop. Reads characters with GetCharPressed() and draws
	/// with raylib text (no raygui text box, so nothing fights for focus).
	///
	/// Enter runs the line, UP / DOWN walk the history, TAB completes, PAGE UP / PAGE DOWN / wheel scroll,
	/// ESC closes (the grave key is handled by the DevConsole).
	/// </summary>
	class ConsoleScreen : public UIScreen
	{
	public:
		explicit ConsoleScreen(DevConsole& console);

		void		OnPush() override;
		void		OnPop() override;
		void		Update(float unscaledDeltaTime) override;
		void		Draw() override;
		const char*	GetName() const override { return "Console"; }

	private:
		DevConsole&	m_console;
		bool		m_wasLocked	= false;
		float		m_blink		= 0.0f;
	};
}

#endif
