#if ENG_DEV_CONSOLE

#include "EngineCommands.h"
#include "DevConsole.h"
#include "core/Application.h"
#include "core/Engine.h"
#include <raylib.h>
#include <algorithm>

namespace eng
{
	namespace
	{
		constexpr int	MAX_EXEC_DEPTH	= 8;
		const char*		MODE_NAMES[]	= { "menu", "play", "pause", "editor" };

		bool ParseMode(const std::string& text, EngineMode& out)
		{
			if (text == "menu")		{ out = EngineMode::MainMenu;	return true; }
			if (text == "play")		{ out = EngineMode::Play;		return true; }
			if (text == "pause")	{ out = EngineMode::Paused;		return true; }
			if (text == "editor")	{ out = EngineMode::Editor;		return true; }
			return false;
		}

		/// No argument toggles, "0" / "1" sets. Returns false (and prints the usage) on anything else.
		bool ParseSwitch(const ConsoleArgs& args, DevConsole& console, bool current, bool& out)
		{
			if (args.Count() == 0)
			{
				out = !current;
				return true;
			}
			const std::string& text = args.GetString(0);
			if (text != "0" && text != "1")
			{
				console.PrintError("usage: %s [0|1]", args.GetCommand().c_str());
				return false;
			}
			out = text == "1";
			return true;
		}

		std::vector<std::string> FilterPrefix(const std::vector<std::string>& all, const std::string& prefix)
		{
			std::vector<std::string> result;
			for (const std::string& name : all)
				if (name.compare(0, prefix.size(), prefix) == 0)
					result.push_back(name);
			return result;
		}

		void RunFile(DevConsole& console, const std::string& name, int depth)
		{
			if (depth > MAX_EXEC_DEPTH)
			{
				console.PrintError("exec: files are nested too deeply (%d)", MAX_EXEC_DEPTH);
				return;
			}

			// "setup" -> setup, setup.cfg, assets/dev/setup.cfg
			std::string candidates[] = { name, name + ".cfg", "assets/dev/" + name, "assets/dev/" + name + ".cfg" };
			for (const std::string& path : candidates)
			{
				if (!FileExists(path.c_str()))
					continue;

				char* text = LoadFileText(path.c_str());
				if (!text)
					break;
				std::string content = text;
				UnloadFileText(text);

				size_t start = 0;
				while (start < content.size())
				{
					size_t end = content.find('\n', start);
					if (end == std::string::npos)
						end = content.size();
					std::string line = content.substr(start, end - start);
					start = end + 1;

					if (!line.empty() && line.back() == '\r')
						line.pop_back();
					const size_t first = line.find_first_not_of(" \t");
					if (first == std::string::npos || line[first] == '#')
						continue;

					if (line.compare(first, 5, "exec ") == 0)
						RunFile(console, ConsoleArgs(line).GetString(0), depth + 1);
					else
						console.Execute(line);
				}
				return;
			}
			console.PrintError("exec: file '%s' not found", name.c_str());
		}
	}

	void RegisterEngineCommands(Engine& engine, DevConsole& console)
	{
		ConsoleCommands& commands = console.GetCommands();

		commands.Register("help", "help [prefix]", "Lists the commands and variables", [](const ConsoleArgs& args, DevConsole& con)
		{
			const std::string& prefix = args.GetString(0);
			int shown = 0;
			for (const auto& [name, command] : con.GetCommands().GetAll())
			{
				if (name.compare(0, prefix.size(), prefix) != 0)
					continue;
				con.Print("  %-28s %s%s", command.usage.c_str(), command.isVar ? "[var] " : "", command.help.c_str());
				++shown;
			}
			if (shown == 0)
				con.PrintError("No command starts with '%s'", prefix.c_str());
		}, [&console](int argIndex, const std::string& prefix) -> std::vector<std::string>
		{
			return argIndex == 0 ? console.GetCommands().Complete(prefix) : std::vector<std::string>{};
		});

		commands.Register("clear", "clear", "Clears the scroll-back", [](const ConsoleArgs&, DevConsole& con)
		{
			con.Clear();
		});

		commands.Register("echo", "echo <text>", "Prints the text", [](const ConsoleArgs& args, DevConsole& con)
		{
			con.PrintLine(args.Rest(0));
		});

		commands.Register("quit", "quit", "Closes the game", [&engine](const ConsoleArgs&, DevConsole&)
		{
			if (Application* app = engine.GetApplication())
				app->SetNeedsToBeClosed(true);
		});

		commands.Register("mode", "mode [menu|play|pause|editor]", "Prints the engine mode, or requests another one",
			[&engine](const ConsoleArgs& args, DevConsole& con)
			{
				if (args.Count() == 0)
				{
					con.Print("mode: %s", ModeName(engine.GetModes().GetMode()));
					return;
				}
				EngineMode target;
				if (!ParseMode(args.GetString(0), target))
				{
					con.PrintError("usage: mode [menu|play|pause|editor]");
					return;
				}
				if (engine.GetModes().Request(target))
					con.Print("mode: requested %s", ModeName(target));
				else
					con.PrintError("mode: %s -> %s is not allowed", ModeName(engine.GetModes().GetMode()), ModeName(target));
			},
			[](int argIndex, const std::string& prefix) -> std::vector<std::string>
			{
				return argIndex == 0 ? FilterPrefix({ std::begin(MODE_NAMES), std::end(MODE_NAMES) }, prefix) : std::vector<std::string>{};
			});

		commands.Register("timescale", "timescale [0..4]", "Prints or sets the time scale", [&engine](const ConsoleArgs& args, DevConsole& con)
		{
			if (args.Count() == 0)
			{
				con.Print("timescale = %g", engine.GetTime().GetTimeScale());
				return;
			}
			float value;
			if (!args.GetFloat(0, value))
			{
				con.PrintError("usage: timescale [0..4]");
				return;
			}
			engine.GetTime().SetTimeScale(std::clamp(value, 0.0f, 4.0f));
			con.Print("timescale = %g", engine.GetTime().GetTimeScale());
		});

		commands.RegisterVar(ConVar::Bool("con_pause", console.GetPauseWhileOpenVar(), "Freeze the simulation while the console is open"));

		commands.Register("phys_debug", "phys_debug [0|1]", "Bullet wireframes", [&engine](const ConsoleArgs& args, DevConsole& con)
		{
			bool value;
			if (!ParseSwitch(args, con, engine.GetPhysics().IsDebugDrawEnabled(), value))
				return;
			engine.GetPhysics().SetDebugDrawEnabled(value);
			con.Print("phys_debug = %d", value ? 1 : 0);
		});

		commands.Register("grid", "grid [0|1]", "Debug grid and debug shapes", [&engine](const ConsoleArgs& args, DevConsole& con)
		{
			bool value;
			if (!ParseSwitch(args, con, engine.GetDebugDraw().IsEnabled(), value))
				return;
			engine.GetDebugDraw().SetEnabled(value);
			con.Print("grid = %d", value ? 1 : 0);
		});

		commands.Register("fps", "fps [0|1]", "FPS and frame time in the corner", [](const ConsoleArgs& args, DevConsole& con)
		{
			bool value;
			if (!ParseSwitch(args, con, con.GetShowFps(), value))
				return;
			con.GetShowFps() = value;
			con.Print("fps = %d", value ? 1 : 0);
		});

		commands.Register("exec", "exec <file>", "Runs a text file of commands, one per line (# starts a comment)", [](const ConsoleArgs& args, DevConsole& con)
		{
			if (args.Count() == 0)
			{
				con.PrintError("usage: exec <file>");
				return;
			}
			RunFile(con, args.GetString(0), 0);
		});
	}
}

#endif
