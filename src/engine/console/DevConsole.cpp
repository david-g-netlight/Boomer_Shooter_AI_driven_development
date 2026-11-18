#if ENG_DEV_CONSOLE

#include "DevConsole.h"
#include "ConsoleScreen.h"
#include "core/Engine.h"
#include "input/InputManager.h"
#include <cstdarg>
#include <cstdio>
#include <raylib.h>

namespace eng
{
	namespace
	{
		constexpr int PRINT_BUFFER_SIZE = 1024;

		DevConsole* g_instance = nullptr;	// target of the Log sink (a plain function pointer)

		std::string CommonPrefix(const std::vector<std::string>& names)
		{
			std::string prefix = names.front();
			for (const std::string& name : names)
			{
				size_t i = 0;
				while (i < prefix.size() && i < name.size() && prefix[i] == name[i])
					++i;
				prefix.resize(i);
			}
			return prefix;
		}
	}

	DevConsole::~DevConsole()
	{
		Shutdown();
	}

	void DevConsole::Init()
	{
		g_instance = this;
		m_installed = true;
		Log::SetSink(&DevConsole::OnLog);
	}

	void DevConsole::Shutdown()
	{
		if (!m_installed)
			return;
		Log::SetSink(nullptr);
		g_instance = nullptr;
		m_installed = false;
	}

	void DevConsole::Open()
	{
		if (!m_screen)
			m_screen = Engine::GetInstance().GetUI().Push<ConsoleScreen>(*this);
	}

	void DevConsole::Close()
	{
		if (m_screen)
			Engine::GetInstance().GetUI().Remove(m_screen);	// OnPop -> NotifyScreenGone
	}

	void DevConsole::Toggle()
	{
		if (IsOpen())	Close();
		else			Open();
	}

	void DevConsole::HandleToggle(const InputManager& input)
	{
		if (input.IsKeyPressed(KEY_GRAVE))
			Toggle();
	}

	void DevConsole::KeepOnTop()
	{
		if (!m_screen || Engine::GetInstance().GetUI().Top() == m_screen)
			return;
		Close();
		Open();
	}

	void DevConsole::NotifyScreenGone(const ConsoleScreen* screen)
	{
		if (m_screen == screen)
			m_screen = nullptr;
	}

	void DevConsole::DrawOverlay() const
	{
		if (!m_showFps)
			return;
		const char* text = TextFormat("%d FPS  %.2f ms", GetFPS(), GetFrameTime() * 1000.0f);
		DrawText(text, GetScreenWidth() - MeasureText(text, 18) - 12, GetScreenHeight() - 28, 18, { 120, 255, 120, 255 });
	}

	void DevConsole::OnLog(LogLevel level, const char* message)
	{
		if (g_instance && level != LogLevel::Debug)
			g_instance->PrintLine(message, level);
	}

	void DevConsole::Execute(const std::string& line)
	{
		ConsoleArgs args(line);
		if (args.GetCommand().empty())
			return;

		AddHistory(line);
		PrintLine("> " + line);

		const ConsoleCommands::Command* command = m_commands.Find(args.GetCommand());
		if (!command)
		{
			PrintError("Unknown command '%s' (try 'help')", args.GetCommand().c_str());
			return;
		}

		// Copy: the handler may unregister commands (and invalidate the table entry) while it runs
		ConsoleCommands::Handler handler = command->handler;
		handler(args, *this);
	}

	void DevConsole::Print(const char* format, ...)
	{
		char buffer[PRINT_BUFFER_SIZE];
		va_list args; va_start(args, format);
		std::vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		PrintLine(buffer, LogLevel::Info);
	}

	void DevConsole::PrintError(const char* format, ...)
	{
		char buffer[PRINT_BUFFER_SIZE];
		va_list args; va_start(args, format);
		std::vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		PrintLine(buffer, LogLevel::Error);
	}

	void DevConsole::PrintLine(const std::string& text, LogLevel level)
	{
		// One output line per '\n' so the scroll-back can draw each on its own row
		size_t start = 0;
		while (start <= text.size())
		{
			size_t end = text.find('\n', start);
			if (end == std::string::npos)
				end = text.size();

			m_lines.push_back({ text.substr(start, end - start), level });
			if (m_lines.size() > MAX_LINES)
				m_lines.pop_front();

			start = end + 1;
		}
	}

	void DevConsole::Clear()
	{
		m_lines.clear();
	}

	void DevConsole::AddHistory(const std::string& line)
	{
		if (m_history.empty() || m_history.back() != line)
		{
			m_history.push_back(line);
			if (m_history.size() > MAX_HISTORY)
				m_history.pop_front();
		}
		HistoryReset();
	}

	const std::string& DevConsole::HistoryPrev()
	{
		static const std::string empty;
		if (m_history.empty())
			return empty;
		if (m_historyCursor > 0)
			--m_historyCursor;
		return m_history[m_historyCursor];
	}

	const std::string& DevConsole::HistoryNext()
	{
		static const std::string empty;
		if (m_historyCursor < (int)m_history.size())
			++m_historyCursor;
		return m_historyCursor < (int)m_history.size() ? m_history[m_historyCursor] : empty;
	}

	void DevConsole::HistoryReset()
	{
		m_historyCursor = (int)m_history.size();
	}

	std::string DevConsole::Complete(const std::string& input)
	{
		// Complete the token under the cursor (the last one); a trailing space starts a new, empty token
		std::vector<std::string> tokens = ConsoleArgs::Tokenize(input);
		bool startsNewToken = input.empty() || input.back() == ' ';
		if (startsNewToken)
			tokens.push_back("");

		std::string prefix = input.substr(input.find_last_of(' ') == std::string::npos ? 0 : input.find_last_of(' ') + 1);
		std::vector<std::string> candidates;

		if (tokens.size() == 1)
			candidates = m_commands.Complete(prefix);
		else if (const ConsoleCommands::Command* command = m_commands.Find(tokens[0]); command && command->completer)
			candidates = command->completer((int)tokens.size() - 2, prefix);

		if (candidates.empty())
			return input;

		if (candidates.size() > 1)
		{
			std::string list;
			for (const std::string& name : candidates)
				list += name + "  ";
			PrintLine(list);
		}

		std::string completed = CommonPrefix(candidates);
		if (candidates.size() == 1)
			completed += ' ';

		std::string result = input.substr(0, input.size() - prefix.size()) + completed;
		return result;
	}
}

#endif
