#if ENG_DEV_CONSOLE && ENG_EDITOR

#include "EditorCommands.h"
#include "DevConsole.h"
#include "core/Engine.h"
#include "editor/ActorEdit.h"
#include "editor/Editor.h"
#include "level/GameActor.h"
#include "level/Level.h"
#include "level/PropertyBag.h"
#include <algorithm>
#include <cctype>
#include <raymath.h>

namespace eng
{
	namespace
	{
		constexpr float MIN_SCALE = 0.01f;

		std::string Lower(std::string text)
		{
			std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
			return text;
		}

		bool RequireEditor(Editor& editor, DevConsole& console, const ConsoleArgs& args)
		{
			if (editor.IsActive())
				return true;
			console.PrintError("%s: only in the editor (type 'editor')", args.GetCommand().c_str());
			return false;
		}

		GameActor* RequireSelection(Editor& editor, DevConsole& console)
		{
			GameActor* selected = editor.GetSelected();
			if (!selected)
				console.PrintError("Nothing selected (select <id|name>, pick)");
			return selected;
		}

		Vector3 GetEulerDegrees(const ::Transform& transform)
		{
			const Vector3 radians = QuaternionToEuler(transform.rotation);
			return { radians.x * RAD2DEG, radians.y * RAD2DEG, radians.z * RAD2DEG };
		}

		std::string ComponentList(const GameActor& actor)
		{
			std::string list;
			for (const auto& component : actor.GetComponents())
			{
				if (!list.empty())
					list += ", ";
				list += component->TypeName();
			}
			return list.empty() ? "-" : list;
		}

		void PrintSelected(DevConsole& console, const GameActor& actor)
		{
			console.Print("selected %u '%s'", (unsigned)actor.GetId(), actor.GetName().c_str());
		}

		/// <summary>"12" (an id) or a name (exact, then a unique case-insensitive match)</summary>
		GameActor* FindActor(Level& level, const std::string& key, DevConsole& console)
		{
			int id;
			if (ConsoleArgs("x " + key).GetInt(0, id))
			{
				if (GameActor* actor = Level::FindActor((ActorId)id); actor && actor->GetLevel() == &level && !actor->IsMarkedForDestroy())
					return actor;
				console.PrintError("No actor with id %d", id);
				return nullptr;
			}

			if (GameActor* actor = level.FindByName(key))
				return actor;

			GameActor* match = nullptr;
			const std::string wanted = Lower(key);
			for (const auto& owned : level.GetActors())
			{
				if (owned->IsMarkedForDestroy() || Lower(owned->GetName()).find(wanted) == std::string::npos)
					continue;
				if (match)
				{
					console.PrintError("'%s' matches more than one actor (use the id, see 'ls')", key.c_str());
					return nullptr;
				}
				match = owned.get();
			}
			if (!match)
				console.PrintError("No actor named '%s'", key.c_str());
			return match;
		}

		std::vector<std::string> ActorNames(Editor& editor, const std::string& prefix)
		{
			std::vector<std::string> names;
			if (Level* level = editor.ResolveLevel())
				for (const auto& owned : level->GetActors())
				{
					const std::string& name = owned->GetName();
					// Names with spaces can't be completed (the line would need quotes): use the id
					if (!owned->IsMarkedForDestroy() && name.find(' ') == std::string::npos && name.compare(0, prefix.size(), prefix) == 0)
						names.push_back(name);
				}
			std::sort(names.begin(), names.end());
			names.erase(std::unique(names.begin(), names.end()), names.end());
			return names;
		}
	}

	void RegisterEditorCommands(Engine& engine, DevConsole& console)
	{
		ConsoleCommands& commands = console.GetCommands();
		Editor& editor = engine.GetEditor();

		//////////////////////////////////////////
		// Mode
		//////////////////////////////////////////
		commands.Register("editor", "editor", "Enters the editor (free camera, the world frozen)", [&engine](const ConsoleArgs&, DevConsole& con)
		{
			if (engine.GetModes().GetMode() == EngineMode::Editor)
				con.Print("Already in the editor");
			else if (engine.GetModes().Request(EngineMode::Editor))
				con.Print("Entering the editor");
			else
				con.PrintError("The editor can't be entered from %s", ModeName(engine.GetModes().GetMode()));
		});

		commands.Register("play", "play", "Leaves the editor, edits stay live", [&engine](const ConsoleArgs&, DevConsole& con)
		{
			if (engine.GetModes().GetMode() == EngineMode::Play)
				con.Print("Already playing");
			else if (engine.GetModes().Request(EngineMode::Play))
				con.Print("Entering Play");
			else
				con.PrintError("Play can't be entered from %s", ModeName(engine.GetModes().GetMode()));
		});

		//////////////////////////////////////////
		// Hierarchy and selection (any mode)
		//////////////////////////////////////////
		commands.Register("ls", "ls [filter]", "Lists the actors: id, name, (parent), components", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			Level* level = editor.ResolveLevel();
			if (!level)
			{
				con.PrintError("No active level");
				return;
			}

			const std::string filter = Lower(args.GetString(0));
			const GameActor* selected = editor.GetSelected();
			int shown = 0;
			for (const auto& owned : level->GetActors())
			{
				const GameActor& actor = *owned;
				if (actor.IsMarkedForDestroy() || Lower(actor.GetName()).find(filter) == std::string::npos)
					continue;

				const std::string parent = actor.GetParent() ? "  (" + actor.GetParent()->GetName() + ")" : "";
				con.Print("%c %4u  %s%s%s  [%s]", &actor == selected ? '*' : ' ', (unsigned)actor.GetId(), actor.GetName().c_str(),
					actor.IsActive() ? "" : " (inactive)", parent.c_str(), ComponentList(actor).c_str());
				++shown;
			}
			con.Print("%d actors", shown);
		});

		commands.Register("select", "select <id|\"name\">", "Selects an actor by id or name", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			Level* level = editor.ResolveLevel();
			if (!level || args.Count() == 0)
			{
				con.PrintError(level ? "usage: select <id|\"name\">" : "No active level");
				return;
			}
			if (GameActor* actor = FindActor(*level, args.Rest(0), con))
			{
				editor.Select(actor);
				PrintSelected(con, *actor);
			}
		}, [&editor](int argIndex, const std::string& prefix) { return argIndex == 0 ? ActorNames(editor, prefix) : std::vector<std::string>{}; });

		commands.Register("pick", "pick", "Selects what is under the middle of the screen (works in Play too)", [&editor](const ConsoleArgs&, DevConsole& con)
		{
			if (GameActor* actor = editor.PickAtScreenCenter())
			{
				editor.Select(actor);
				PrintSelected(con, *actor);
			}
			else
				con.Print("Nothing under the crosshair");
		});

		commands.Register("deselect", "deselect", "Clears the selection", [&editor](const ConsoleArgs&, DevConsole&)
		{
			editor.Select(nullptr);
		});

		commands.Register("info", "info", "Transform and components of the selection", [&editor](const ConsoleArgs&, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;

			const ::Transform& t = actor->GetTransform();
			const Vector3 euler = GetEulerDegrees(t);
			con.Print("%u '%s'%s%s", (unsigned)actor->GetId(), actor->GetName().c_str(), actor->IsActive() ? "" : "  (inactive)",
				actor->GetParent() ? ("  parent '" + actor->GetParent()->GetName() + "'").c_str() : "");
			con.Print("  pos %g %g %g  rot %g %g %g  scale %g %g %g", (double)t.translation.x, (double)t.translation.y, (double)t.translation.z,
				(double)euler.x, (double)euler.y, (double)euler.z, (double)t.scale.x, (double)t.scale.y, (double)t.scale.z);

			for (const auto& component : actor->GetComponents())
			{
				PropertyBag bag;
				component->Serialize(bag);
				std::string fields;
				for (const auto& [key, value] : bag.Items())
					fields += " " + key + "=" + value;
				con.Print("  %s%s%s", component->TypeName(), component->IsEnabled() ? "" : " (disabled)", fields.c_str());
			}
		});

		//////////////////////////////////////////
		// Editing the selection
		//////////////////////////////////////////
		commands.Register("rename", "rename \"<new name>\"", "Renames the selection", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;
			if (args.Count() == 0)
			{
				con.PrintError("usage: rename \"<new name>\"");
				return;
			}
			actor->SetName(args.Rest(0));
			con.Print("renamed to '%s'", actor->GetName().c_str());
		});

		commands.Register("pos", "pos <x> <y> <z>", "Sets the local position", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;
			Vector3 value;
			if (!args.GetVector3(0, value))
			{
				con.PrintError("usage: pos <x> <y> <z>");
				return;
			}
			::Transform transform = actor->GetTransform();
			transform.translation = value;
			ActorEdit::SetLocalTransform(*actor, transform);
		});

		commands.Register("move", "move <dx> <dy> <dz>", "Moves the selection by an offset (local space)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;
			Vector3 value;
			if (!args.GetVector3(0, value))
			{
				con.PrintError("usage: move <dx> <dy> <dz>");
				return;
			}
			::Transform transform = actor->GetTransform();
			transform.translation = Vector3Add(transform.translation, value);
			ActorEdit::SetLocalTransform(*actor, transform);
		});

		commands.Register("rot", "rot <pitch> <yaw> <roll>", "Sets the rotation, euler degrees", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;
			Vector3 degrees;
			if (!args.GetVector3(0, degrees))
			{
				con.PrintError("usage: rot <pitch> <yaw> <roll>");
				return;
			}
			::Transform transform = actor->GetTransform();
			transform.rotation = QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
			ActorEdit::SetLocalTransform(*actor, transform);
		});

		commands.Register("scale", "scale <x> <y> <z> | scale <s>", "Sets the local scale (minimum 0.01)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;

			Vector3 value;
			float uniform;
			if (args.Count() == 1 && args.GetFloat(0, uniform))
				value = { uniform, uniform, uniform };
			else if (!args.GetVector3(0, value))
			{
				con.PrintError("usage: scale <x> <y> <z> | scale <s>");
				return;
			}
			::Transform transform = actor->GetTransform();
			transform.scale = { std::max(value.x, MIN_SCALE), std::max(value.y, MIN_SCALE), std::max(value.z, MIN_SCALE) };
			ActorEdit::SetLocalTransform(*actor, transform);
		});

		commands.Register("active", "active <0|1>", "Enables or disables the selection", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			if (!actor)
				return;
			const std::string& text = args.GetString(0);
			if (text != "0" && text != "1")
			{
				con.PrintError("usage: active <0|1>");
				return;
			}
			actor->SetActive(text == "1");
		});

		commands.Register("parent", "parent <id|\"name\"|none>", "Parents the selection, keeping its world pose", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			GameActor* actor = RequireSelection(editor, con);
			Level* level = editor.ResolveLevel();
			if (!actor || !level)
				return;
			if (args.Count() == 0)
			{
				con.PrintError("usage: parent <id|\"name\"|none>");
				return;
			}

			GameActor* parent = nullptr;
			if (args.GetString(0) != "none")
			{
				parent = FindActor(*level, args.Rest(0), con);
				if (!parent)
					return;
			}
			if (!level->SetParent(actor, parent, true))
				con.PrintError("Can't parent '%s' there (itself or one of its children)", actor->GetName().c_str());
		}, [&editor](int argIndex, const std::string& prefix) { return argIndex == 0 ? ActorNames(editor, prefix) : std::vector<std::string>{}; });

		//////////////////////////////////////////
		// Creating, copying, deleting (editor only: they use the editor camera)
		//////////////////////////////////////////
		commands.Register("add", "add cube|trigger|empty", "Creates an actor 4 m in front of the editor camera", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			const std::string& what = args.GetString(0);
			GameActor* actor = nullptr;
			if (what == "cube")			actor = editor.AddCube();
			else if (what == "trigger")	actor = editor.AddTrigger();
			else if (what == "empty")	actor = editor.AddEmpty();
			else
			{
				con.PrintError("usage: add cube|trigger|empty");
				return;
			}
			if (actor)
				PrintSelected(con, *actor);
		}, [](int argIndex, const std::string& prefix) -> std::vector<std::string>
		{
			std::vector<std::string> result;
			if (argIndex == 0)
				for (const char* name : { "cube", "trigger", "empty" })
					if (std::string(name).compare(0, prefix.size(), prefix) == 0)
						result.push_back(name);
			return result;
		});

		commands.Register("dup", "dup", "Duplicates the selection (with children) and selects the copy", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			if (!RequireSelection(editor, con))
				return;
			if (GameActor* copy = editor.DuplicateSelected())
				PrintSelected(con, *copy);
			else
				con.PrintError("dup: the copy failed (see the log above)");
		});

		commands.Register("del", "del", "Deletes the selection (with children)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			if (RequireSelection(editor, con))
				editor.DeleteSelected();
		});

		commands.Register("focus", "focus", "Editor camera to the selection (F)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			if (RequireSelection(editor, con))
				editor.FocusSelected();
		});

		//////////////////////////////////////////
		// Spawn point, snapshot, files
		//////////////////////////////////////////
		commands.Register("spawn_here", "spawn_here", "Spawn point = on the ground below the editor camera, facing where it looks", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			editor.SetSpawnHere();
		});

		commands.Register("spawn", "spawn <x> <y> <z> [yaw]", "Sets the spawn point (feet position, yaw degrees)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			Level* level = editor.ResolveLevel();
			Vector3 feet;
			if (!level || !args.GetVector3(0, feet))
			{
				con.PrintError(level ? "usage: spawn <x> <y> <z> [yaw]" : "No active level");
				return;
			}
			float yaw = level->GetSpawn().yawDegrees;
			args.GetFloat(3, yaw);
			level->SetSpawn(feet, yaw);
			con.Print("spawn = %g %g %g  yaw %g", (double)feet.x, (double)feet.y, (double)feet.z, (double)yaw);
		});

		commands.Register("reset", "reset", "Restores the level to how it was when the editor was entered", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			editor.Reset();
		});

		commands.Register("save", "save [path]", "Saves the level (default: the game's level file)", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			editor.Save(args.GetString(0));
			con.Print("saved %s", args.Count() ? args.GetString(0).c_str() : "the level file");
		});

		commands.Register("load", "load [path]", "Loads the level file at the start of the next editor frame", [&editor](const ConsoleArgs& args, DevConsole& con)
		{
			if (!RequireEditor(editor, con, args))
				return;
			editor.RequestLoad(args.GetString(0));
			con.Print("loading %s", args.Count() ? args.GetString(0).c_str() : "the level file");
		});
	}
}

#endif
