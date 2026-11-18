#if ENG_DEV_CONSOLE

#include "ConsoleScreen.h"
#include "DevConsole.h"
#include "core/Engine.h"
#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace eng
{
	namespace
	{
		constexpr int		FONT_SIZE		= 18;
		constexpr int		LINE_HEIGHT		= 22;
		constexpr int		PADDING			= 10;
		constexpr float		HEIGHT_FRACTION	= 0.5f;
		constexpr int		PAGE_LINES		= 8;
		constexpr int		WHEEL_LINES		= 3;
		constexpr float		BLINK_PERIOD	= 1.0f;

		Color LineColor(const DevConsole::Line& line)
		{
			if (line.text.rfind("> ", 0) == 0)		return { 130, 200, 255, 255 };	// an echoed command
			switch (line.level)
			{
				case LogLevel::Warning:	return { 255, 210, 90, 255 };
				case LogLevel::Error:	return { 255, 110, 110, 255 };
				default:				return { 215, 215, 215, 255 };
			}
		}
	}

	ConsoleScreen::ConsoleScreen(DevConsole& console)
		: m_console(console)
	{
	}

	void ConsoleScreen::OnPush()
	{
		InputManager& input = Engine::GetInstance().GetInputManager();
		m_wasLocked = input.IsCursorLocked();
		input.SetCursorLocked(false);

		while (GetCharPressed() != 0) {}	// the grave key that opened the console is still in the queue
		m_console.GetScroll() = 0;
		m_console.HistoryReset();
	}

	void ConsoleScreen::OnPop()
	{
		Engine& engine = Engine::GetInstance();
		// Only Play locks the cursor, whatever the state was when the console opened
		if (m_wasLocked && engine.GetModes().GetMode() == EngineMode::Play)
			engine.GetInputManager().SetCursorLocked(true);

		m_console.NotifyScreenGone(this);
	}

	void ConsoleScreen::Update(float unscaledDeltaTime)
	{
		m_blink = std::fmod(m_blink + unscaledDeltaTime, BLINK_PERIOD);

		std::string& input = m_console.GetInput();

		for (int c = GetCharPressed(); c != 0; c = GetCharPressed())
			if (c >= 32 && c < 127 && c != '`')
				input += (char)c;

		if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !input.empty())
			input.pop_back();

		if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))
		{
			std::string line = input;
			input.clear();
			m_console.GetScroll() = 0;
			m_console.Execute(line);
		}

		if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))		input = m_console.HistoryPrev();
		if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))	input = m_console.HistoryNext();
		if (IsKeyPressed(KEY_TAB))									input = m_console.Complete(input);

		int& scroll = m_console.GetScroll();
		if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP))		scroll += PAGE_LINES;
		if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN))	scroll -= PAGE_LINES;
		scroll += (int)(GetMouseWheelMove() * WHEEL_LINES);
		scroll = std::clamp(scroll, 0, std::max(0, (int)m_console.GetLines().size() - 1));

		if (IsKeyPressed(KEY_ESCAPE))
			m_console.Close();
	}

	void ConsoleScreen::Draw()
	{
		const int width		= GetScreenWidth();
		const int height	= (int)(GetScreenHeight() * HEIGHT_FRACTION);

		DrawRectangle(0, 0, width, height, { 12, 14, 20, 225 });
		DrawRectangle(0, height, width, 2, { 90, 170, 255, 255 });

		// Newest line sits just above the input row; `scroll` lines are hidden below
		const std::deque<DevConsole::Line>& lines = m_console.GetLines();
		const int inputY	= height - LINE_HEIGHT - PADDING / 2;
		const int visible	= std::max(0, (inputY - PADDING) / LINE_HEIGHT);
		const int newest	= (int)lines.size() - 1 - m_console.GetScroll();

		for (int row = 0; row < visible; ++row)
		{
			const int index = newest - row;
			if (index < 0)
				break;
			DrawText(lines[index].text.c_str(), PADDING, inputY - (row + 1) * LINE_HEIGHT, FONT_SIZE, LineColor(lines[index]));
		}

		DrawRectangle(0, inputY - 2, width, LINE_HEIGHT + 2, { 30, 34, 46, 255 });
		const std::string prompt = "> " + m_console.GetInput();
		DrawText(prompt.c_str(), PADDING, inputY, FONT_SIZE, WHITE);
		if (m_blink < BLINK_PERIOD * 0.5f)
			DrawRectangle(PADDING + MeasureText(prompt.c_str(), FONT_SIZE) + 2, inputY, 9, FONT_SIZE, WHITE);

		if (m_console.GetScroll() > 0)
			DrawText("-- scrolled back --", width - 200, PADDING, 14, { 255, 255, 255, 120 });
	}
}

#endif
