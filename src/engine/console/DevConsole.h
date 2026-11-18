#pragma once
#if ENG_DEV_CONSOLE

#include "ConsoleCommands.h"
#include "core/Log.h"
#include <deque>
#include <string>
#include <vector>

namespace eng
{
	/// <summary>
	/// The developer console: command table, scroll-back and history. Owned by the Engine.
	/// Every Log line is mirrored into the scroll-back while the console is installed.
	/// </summary>
	class ConsoleScreen;
	class InputManager;

	class DevConsole
	{
	public:
		struct Line
		{
			std::string	text;
			LogLevel	level = LogLevel::Info;
		};

		static constexpr size_t MAX_LINES		= 512;
		static constexpr size_t MAX_HISTORY		= 64;

		DevConsole() = default;
		~DevConsole();

		DevConsole(const DevConsole&)				= delete;
		DevConsole& operator = (const DevConsole&)	= delete;

		/// <summary>Installs the Log sink</summary>
		void Init();
		void Shutdown();

		//////////////////////////////////////////////////////////////
		// Open / close: the ConsoleScreen goes on the UIStack while the console is open
		//////////////////////////////////////////////////////////////
		bool	IsOpen() const { return m_screen != nullptr; }
		void	Open();
		void	Close();
		void	Toggle();
		/// <summary>Grave key, once per frame (Engine)</summary>
		void	HandleToggle(const InputManager& input);
		/// <summary>After a mode change pushed its own screen: the console goes back on top</summary>
		void	KeepOnTop();
		/// <summary>Called by the ConsoleScreen when it leaves the stack</summary>
		void	NotifyScreenGone(const ConsoleScreen* screen);

		/// <summary>True while the console is open and the simulation is frozen (cvar con_pause)</summary>
		bool	IsPausingSimulation() const { return IsOpen() && m_pauseWhileOpen; }
		bool*	GetPauseWhileOpenVar() { return &m_pauseWhileOpen; }

		std::string&	GetInput()		{ return m_input; }	// the line being typed, kept while the screen is re-pushed
		int&			GetScroll()		{ return m_scroll; }	// lines scrolled back from the newest

		/// <summary>Runs one line: finds the command and calls its handler. The line goes into the history.</summary>
		void Execute(const std::string& line);

		void Print		(const char* format, ...) ENG_PRINTF_FORMAT(2, 3);
		void PrintError	(const char* format, ...) ENG_PRINTF_FORMAT(2, 3);
		void PrintLine	(const std::string& text, LogLevel level = LogLevel::Info);
		void Clear();

		const std::deque<Line>&		GetLines() const { return m_lines; }
		ConsoleCommands&			GetCommands() { return m_commands; }

		//////////////////////////////////////////////////////////////
		// History (oldest first). Navigation: Prev goes back in time, Next forward; Next past the newest returns "".
		//////////////////////////////////////////////////////////////
		const std::deque<std::string>&	GetHistory() const { return m_history; }
		const std::string&				HistoryPrev();
		const std::string&				HistoryNext();
		void							HistoryReset();

		/// <summary>
		/// Completes `input` (the whole input line). Returns the new input text; when several candidates remain
		/// they are printed and the input extends to their common prefix.
		/// </summary>
		std::string Complete(const std::string& input);

	private:
		static void OnLog(LogLevel level, const char* message);
		void AddHistory(const std::string& line);

		ConsoleCommands			m_commands;
		std::deque<Line>		m_lines;
		std::deque<std::string>	m_history;
		int						m_historyCursor = 0;	// == history size when not navigating
		bool					m_installed		= false;
		ConsoleScreen*			m_screen		= nullptr;	// non-owning: the UIStack owns it while open
		bool					m_pauseWhileOpen = true;
		std::string				m_input;
		int						m_scroll		= 0;
	};
}

#endif
