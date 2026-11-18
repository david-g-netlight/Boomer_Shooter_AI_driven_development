#if ENG_DEV_CONSOLE

#include "ConsoleCommands.h"
#include "DevConsole.h"
#include "core/Log.h"
#include <memory>

namespace eng
{
	ConsoleCommands::Token ConsoleCommands::Register(const char* name, const char* usage, const char* help, Handler handler, Completer completer)
	{
		if (m_commands.count(name))
		{
			Log::Warning("Console command '%s' is already registered", name);
			return INVALID_TOKEN;
		}

		Command command;
		command.token		= m_nextToken++;
		command.name		= name;
		command.usage		= usage;
		command.help		= help;
		command.handler		= std::move(handler);
		command.completer	= std::move(completer);
		m_commands[name]	= std::move(command);
		return m_commands[name].token;
	}

	ConsoleCommands::Token ConsoleCommands::RegisterVar(ConVar var)
	{
		std::string name	= var.GetName();
		std::string usage	= var.GetUsage();
		std::string help	= var.GetHelp();

		// The handler owns the variable binding, so it lives exactly as long as the registration
		auto shared = std::make_shared<ConVar>(std::move(var));
		Token token = Register(name.c_str(), usage.c_str(), help.c_str(),
			[shared](const ConsoleArgs& args, DevConsole& console)
			{
				if (args.Count() == 0)
				{
					console.Print("%s = %s", shared->GetName().c_str(), shared->GetValueText().c_str());
					return;
				}
				std::string error;
				if (shared->SetFromText(args.GetString(0), error))
					console.Print("%s = %s", shared->GetName().c_str(), shared->GetValueText().c_str());
				else
					console.PrintError("%s: %s (usage: %s)", shared->GetName().c_str(), error.c_str(), shared->GetUsage().c_str());
			});

		if (token != INVALID_TOKEN)
			m_commands[name].isVar = true;
		return token;
	}

	void ConsoleCommands::Unregister(Token token)
	{
		if (token == INVALID_TOKEN)
			return;

		for (auto it = m_commands.begin(); it != m_commands.end(); ++it)
		{
			if (it->second.token == token)
			{
				m_commands.erase(it);
				return;
			}
		}
	}

	const ConsoleCommands::Command* ConsoleCommands::Find(const std::string& name) const
	{
		auto it = m_commands.find(name);
		return it == m_commands.end() ? nullptr : &it->second;
	}

	std::vector<std::string> ConsoleCommands::Complete(const std::string& prefix) const
	{
		std::vector<std::string> result;
		for (auto it = m_commands.lower_bound(prefix); it != m_commands.end(); ++it)
		{
			if (it->first.compare(0, prefix.size(), prefix) != 0)
				break;
			result.push_back(it->first);
		}
		return result;
	}
}

#endif
