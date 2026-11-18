#pragma once
#if ENG_DEV_CONSOLE

#include "ConsoleArgs.h"
#include "ConVar.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace eng
{
	class DevConsole;

	/// <summary>
	/// Table: name -> { help, usage, handler, completer }. Engine, editor and game register commands by name and
	/// keep the token to unregister them (a handler that captures `this` must not outlive its owner).
	/// Names are lowercase snake_case. A ConVar registers as a command named after the variable.
	/// </summary>
	class ConsoleCommands
	{
	public:
		using Token		= unsigned int;
		using Handler	= std::function<void(const ConsoleArgs& args, DevConsole& console)>;
		/// <summary>Candidates for argument `argIndex` (0 = first argument) that start with `prefix`</summary>
		using Completer	= std::function<std::vector<std::string>(int argIndex, const std::string& prefix)>;

		static constexpr Token INVALID_TOKEN = 0;

		struct Command
		{
			Token		token = INVALID_TOKEN;
			std::string	name;
			std::string	usage;
			std::string	help;
			Handler		handler;
			Completer	completer;
			bool		isVar = false;
		};

		Token			Register(const char* name, const char* usage, const char* help, Handler handler, Completer completer = nullptr);
		/// <summary>The variable must outlive the registration</summary>
		Token			RegisterVar(ConVar var);
		void			Unregister(Token token);

		const Command*	Find(const std::string& name) const;

		/// <summary>Command names starting with `prefix`, sorted</summary>
		std::vector<std::string> Complete(const std::string& prefix) const;

		/// <summary>All commands, sorted by name</summary>
		const std::map<std::string, Command>& GetAll() const { return m_commands; }

	private:
		std::map<std::string, Command>	m_commands;
		Token							m_nextToken = 1;
	};
}

#endif
