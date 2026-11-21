#if ENG_DEV_CONSOLE

#include "GameCommands.h"
#include "flow/GameplayManager.h"
#include "player/PlayerActor.h"
#include "state/GameState.h"
#include "state/PlayerState.h"
#include "state/Settings.h"

namespace
{
	/// The player actor is placed by its capsule center: the feet are this far below
	constexpr float FEET_TO_CENTER = PlayerActor::CAPSULE_HEIGHT * 0.5f + PlayerActor::CAPSULE_RADIUS;

	bool TeleportPlayer(const GameCommandContext& context, const Vector3& feet)
	{
		eng::GameActor* actor = context.player ? context.player->Get() : nullptr;
		eng::CharacterMotor* motor = actor ? actor->GetComponent<eng::CharacterMotor>() : nullptr;
		if (!motor)
			return false;
		motor->Warp({ feet.x, feet.y + FEET_TO_CENTER, feet.z });
		return true;
	}
}

void GameCommands::Register(const GameCommandContext& context)
{
	Unregister();
	eng::ConsoleCommands& commands = eng::Engine::GetInstance().GetConsole().GetCommands();
	auto add = [&](const char* name, const char* usage, const char* help, eng::ConsoleCommands::Handler handler,
				   eng::ConsoleCommands::Completer completer = nullptr)
	{
		m_tokens.push_back(commands.Register(name, usage, help, std::move(handler), std::move(completer)));
	};

	//////////////////////////////////////////
	// Level flow (requests)
	//////////////////////////////////////////
	add("restart", "restart", "Reloads the current level", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		context.gameplay->ReloadCurrentLevel();
		con.Print("restarting level %d", context.gameplay->GetLevelIndex());
	});

	add("level_list", "level_list", "Lists the levels (* = current)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		const LevelList& levels = context.gameplay->GetLevels();
		for (size_t i = 0; i < levels.GetCount(); ++i)
			con.Print("%c %zu  %s", (int)i == context.gameplay->GetLevelIndex() ? '*' : ' ', i, levels.GetPath((int)i).c_str());
	});

	add("level_load", "level_load <index|path>", "Loads a level of the list", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		const LevelList& levels = context.gameplay->GetLevels();
		int index = -1;
		if (args.Count() == 0)
		{
			con.PrintError("usage: level_load <index|path>");
			return;
		}
		if (!args.GetInt(0, index))
			for (size_t i = 0; i < levels.GetCount(); ++i)
				if (levels.GetPath((int)i) == args.GetString(0))
					index = (int)i;

		if (!levels.IsValid(index))
		{
			con.PrintError("level_load: '%s' is not in the level list (see level_list)", args.GetString(0).c_str());
			return;
		}
		context.gameplay->LoadLevel(index);
		con.Print("loading level %d", index);
	}, [context](int argIndex, const std::string& prefix) -> std::vector<std::string>
	{
		std::vector<std::string> result;
		const LevelList& levels = context.gameplay->GetLevels();
		for (size_t i = 0; argIndex == 0 && i < levels.GetCount(); ++i)
			if (levels.GetPath((int)i).compare(0, prefix.size(), prefix) == 0)
				result.push_back(levels.GetPath((int)i));
		return result;
	});

	add("level_next", "level_next", "Loads the next level of the list", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		if (context.gameplay->LoadNextLevel())
			con.Print("loading level %d", context.gameplay->GetLevelIndex() + 1);
		else
			con.Print("This is the last level");
	});

	add("win", "win", "Wins the level (while playing)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		if (!context.gameState->WinGame())
			con.PrintError("win: the game is %s, not Playing", GameState::StatusName(context.gameState->GetStatus()));
	});

	add("lose", "lose", "Loses the level (while playing)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		if (!context.gameState->LoseGame())
			con.PrintError("lose: the game is %s, not Playing", GameState::StatusName(context.gameState->GetStatus()));
	});

	//////////////////////////////////////////
	// The player
	//////////////////////////////////////////
	add("tp", "tp <x> <y> <z>", "Teleports the player (feet position)", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		Vector3 feet;
		if (!args.GetVector3(0, feet))
			con.PrintError("usage: tp <x> <y> <z>");
		else if (!TeleportPlayer(context, feet))
			con.PrintError("tp: there is no player");
	});

	add("tp_spawn", "tp_spawn", "Teleports the player to the level's spawn point", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		const eng::SpawnPoint& spawn = context.level->GetSpawn();
		if (!spawn.isSet)
			con.PrintError("tp_spawn: this level has no spawn point");
		else if (!TeleportPlayer(context, spawn.position))
			con.PrintError("tp_spawn: there is no player");
	});

	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("god", context.playerState->GetGodFlag(), "Invulnerable (read by the damage code, once there is some)")));

	add("gun_cooldown", "gun_cooldown [seconds]", "Prints or sets the time between shots", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		float value;
		if (args.Count() > 0 && !args.GetFloat(0, value))
		{
			con.PrintError("usage: gun_cooldown [seconds]");
			return;
		}
		if (args.Count() > 0)
			context.playerState->SetCooldown(value);
		con.Print("gun_cooldown = %g", (double)context.playerState->GetCooldown());
	});

	add("gun_reach", "gun_reach [meters]", "Prints or sets how far the gun reaches", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		float value;
		if (args.Count() > 0 && !args.GetFloat(0, value))
		{
			con.PrintError("usage: gun_reach [meters]");
			return;
		}
		if (args.Count() > 0)
			context.playerState->SetReach(value);
		con.Print("gun_reach = %g", (double)context.playerState->GetReach());
	});

	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Float("sens", &Settings::Instance().mouseSensitivity,
		Settings::MIN_SENSITIVITY, Settings::MAX_SENSITIVITY, "Mouse sensitivity multiplier")));

#ifndef NDEBUG
	add("ecs_selftest", "ecs_selftest", "Runs the ECS storage self-check", [](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		std::vector<std::string> failures = eng::ecs::RunRegistrySelfTest();
		if (failures.empty())
			con.Print("ecs_selftest: all checks passed");
		for (const std::string& failure : failures)
			con.PrintError("ecs_selftest FAILED: %s", failure.c_str());
	});
#endif
}

void GameCommands::Unregister()
{
	eng::ConsoleCommands& commands = eng::Engine::GetInstance().GetConsole().GetCommands();
	for (eng::ConsoleCommands::Token token : m_tokens)
		commands.Unregister(token);
	m_tokens.clear();
}

#endif
