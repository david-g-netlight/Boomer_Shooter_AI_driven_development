#include "MenuScreens.h"
#include "ui/UIStyle.h"
#include "core/Application.h"
#include "core/Engine.h"
#include "core/EngineInfo.h"
#include <raygui.h>

namespace eng
{
	namespace
	{
		void Quit()
		{
			if (Application* app = Engine::GetInstance().GetApplication())
				app->SetNeedsToBeClosed(true);
		}

		void CenteredText(const char* text, float y, int size, Color color)
		{
			DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, static_cast<int>(y), size, color);
		}
	}

	//////////////////////////////////////////
	/// Main menu
	//////////////////////////////////////////
	void MainMenuScreen::Draw()
	{
		Engine&			engine	= Engine::GetInstance();
		ModeMachine&	modes	= engine.GetModes();
		const float		top		= 0.25f * static_cast<float>(GetScreenHeight());

		UIStyle::DimBackground(0.25f);
		UIStyle::DrawTitle(engine.GetApplication() ? engine.GetApplication()->GetTitle() : GetEngineName(), top);
		CenteredText(TextFormat("%s %s", GetEngineName(), GetEngineVersion()), top + 60.0f, 20, Fade(RAYWHITE, 0.8f));

		int index = 0;
		const float buttons = top + 120.0f;
		if (GuiButton(UIStyle::ButtonRect(index++, buttons), "Start"))
			modes.Request(EngineMode::Play);
		if (IsEditorBuild() && GuiButton(UIStyle::ButtonRect(index++, buttons), "Editor"))
			modes.Request(EngineMode::Editor);
		if (GuiButton(UIStyle::ButtonRect(index++, buttons), "Quit"))
			Quit();

		CenteredText("ESC pauses the game", static_cast<float>(GetScreenHeight()) - 40.0f, 16, Fade(RAYWHITE, 0.7f));
	}

	//////////////////////////////////////////
	/// Pause menu
	//////////////////////////////////////////
	void PauseMenuScreen::Draw()
	{
		ModeMachine&	modes	= Engine::GetInstance().GetModes();
		const float		top		= 0.25f * static_cast<float>(GetScreenHeight());

		UIStyle::DimBackground(0.55f);
		UIStyle::DrawTitle("Paused", top);

		const float buttons = top + 100.0f;
		if (GuiButton(UIStyle::ButtonRect(0, buttons), "Resume"))		modes.Request(EngineMode::Play);
		if (GuiButton(UIStyle::ButtonRect(1, buttons), "Restart"))		modes.RequestRestart();
		if (GuiButton(UIStyle::ButtonRect(2, buttons), "Main Menu"))	modes.Request(EngineMode::MainMenu);
		if (GuiButton(UIStyle::ButtonRect(3, buttons), "Quit"))			Quit();

		CenteredText("ESC to resume", static_cast<float>(GetScreenHeight()) - 40.0f, 16, Fade(RAYWHITE, 0.7f));
	}

	//////////////////////////////////////////
	/// Message dialog
	//////////////////////////////////////////
	MessageScreen::MessageScreen(std::string title, std::string message, std::vector<Button> buttons)
		: m_title(std::move(title))
		, m_message(std::move(message))
		, m_buttons(std::move(buttons))
	{
		if (m_buttons.empty())
			m_buttons.push_back({ "OK", {} });
	}

	void MessageScreen::OnPush()
	{
		Engine::GetInstance().GetInputManager().SetCursorLocked(false);
	}

	void MessageScreen::OnPop()
	{
		Engine& engine = Engine::GetInstance();
		if (engine.GetModes().GetMode() == EngineMode::Play)
			engine.GetInputManager().SetCursorLocked(true);
	}

	void MessageScreen::Draw()
	{
		const float top = 0.3f * static_cast<float>(GetScreenHeight());

		UIStyle::DimBackground(0.45f);
		UIStyle::DrawTitle(m_title.c_str(), top);
		CenteredText(m_message.c_str(), top + 64.0f, 20, RAYWHITE);

		const float buttons = top + 110.0f;
		for (size_t i = 0; i < m_buttons.size(); ++i)
		{
			if (!GuiButton(UIStyle::ButtonRect(static_cast<int>(i), buttons), m_buttons[i].label.c_str()))
				continue;

			// Copy: the callback may destroy this screen (it is only removed after Draw)
			const std::function<void()> onClick = m_buttons[i].onClick;
			Engine::GetInstance().GetUI().Remove(this);
			if (onClick)
				onClick();
			break;
		}
	}

	//////////////////////////////////////////
	/// Editor placeholder
	//////////////////////////////////////////
	void EditorBannerScreen::Draw()
	{
		DrawRectangle(0, 0, GetScreenWidth(), 36, Fade(DARKBLUE, 0.85f));
		CenteredText("EDITOR  -  RMB + WASD/QE fly  -  LMB select  -  W move / R scale  -  F focus  -  Del delete  -  TAB back to the game", 10.0f, 16, RAYWHITE);
	}
}
