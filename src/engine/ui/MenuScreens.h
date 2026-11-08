#pragma once
#include <functional>
#include <string>
#include <vector>
#include "ui/UIScreen.h"

namespace eng
{
	/// <summary>
	/// Shown in EngineMode::MainMenu: game title over the 3D backdrop, Start / Editor (editor builds) / Quit.
	/// </summary>
	class MainMenuScreen : public UIScreen
	{
	public:
		void		Draw() override;
		bool		IsOpaque() const override { return true; }
		const char*	GetName() const override { return "MainMenu"; }
	};

	/// <summary>
	/// Shown in EngineMode::Paused over the frozen frame: Resume / Restart / Main Menu / Quit.
	/// </summary>
	class PauseMenuScreen : public UIScreen
	{
	public:
		void		Draw() override;
		bool		IsOpaque() const override { return true; }
		const char*	GetName() const override { return "Paused"; }
	};

	/// <summary>
	/// Generic dialog: title, message, buttons with callbacks (win / lose, confirmations).
	/// Frees the cursor while shown; re-locks it on close if the engine is still in Play.
	/// A button closes the dialog after its callback unless the callback pushed / removed screens itself.
	/// </summary>
	class MessageScreen : public UIScreen
	{
	public:
		struct Button
		{
			std::string				label;
			std::function<void()>	onClick;	// may be empty: just closes
		};

		MessageScreen(std::string title, std::string message, std::vector<Button> buttons);

		void		OnPush() override;
		void		OnPop() override;
		void		Draw() override;
		const char*	GetName() const override { return "Message"; }

	private:
		std::string			m_title;
		std::string			m_message;
		std::vector<Button>	m_buttons;
	};

	/// <summary>
	/// TODO(Phase 9): placeholder banner while the editor UI doesn't exist yet.
	/// </summary>
	class EditorBannerScreen : public UIScreen
	{
	public:
		void		Draw() override;
		const char*	GetName() const override { return "EditorBanner"; }
	};
}
