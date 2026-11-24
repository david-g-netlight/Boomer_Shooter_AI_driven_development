#if ENG_DEV_CONSOLE

#include "GameCommands.h"
#include <algorithm>
#include <cstdint>
#include <vector>
#include "enemies/EnemyComponents.h"
#include "enemies/EnemyManager.h"
#include "flow/GameplayManager.h"
#include "player/GunView.h"
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

	/// The player's position and the planar direction they face (the body's yaw)
	bool GetPlayerPose(const GameCommandContext& context, Vector3& position, Vector3& forward)
	{
		eng::GameActor* actor = context.player ? context.player->Get() : nullptr;
		if (!actor)
			return false;
		position = actor->GetWorldPosition();
		forward = Vector3RotateByQuaternion({ 0.0f, 0.0f, -1.0f }, actor->GetTransform().rotation);
		forward.y = 0.0f;
		forward = Vector3Normalize(forward);
		return true;
	}

	/// A grid in front of the player: up to 5 columns, the first row 6 m out, 2 m between rows, 1.5 m between columns
	Vector3 SpawnSlot(const Vector3& origin, const Vector3& forward, int index, int total)
	{
		constexpr int	COLUMNS			= 5;
		constexpr float	FRONT			= 6.0f;
		constexpr float	ROW_SPACING		= 2.0f;
		constexpr float	COLUMN_SPACING	= 1.5f;

		const int columns	= std::min(total, COLUMNS);
		const float column	= static_cast<float>(index % columns) - (columns - 1) * 0.5f;
		const float row		= static_cast<float>(index / columns);
		const Vector3 right	= Vector3CrossProduct(forward, { 0.0f, 1.0f, 0.0f });
		return Vector3Add(origin, Vector3Add(Vector3Scale(forward, FRONT + row * ROW_SPACING), Vector3Scale(right, column * COLUMN_SPACING)));
	}

	/// Spawns one enemy per entry, facing the player. Returns how many spawned (less than the list when the pool is full),
	/// or -1 when there is no player.
	int SpawnInFront(const GameCommandContext& context, const std::vector<enemies::EnemyType>& types)
	{
		Vector3 position{};
		Vector3 forward{};
		if (!GetPlayerPose(context, position, forward))
			return -1;

		static uint32_t seed = 1;	// a new seed per spawn, so the wander RNGs differ
		const int total = static_cast<int>(types.size());
		int spawned = 0;
		for (; spawned < total; ++spawned)
		{
			seed = seed * 1664525u + 1013904223u;
			const eng::ecs::Entity e = context.enemyManager->GetPool().Spawn(types[spawned], SpawnSlot(position, forward, spawned, total), Vector3Negate(forward), seed);
			if (e == eng::ecs::INVALID_ENTITY)
				break;
		}
		return spawned;
	}

	const char* StateName(enemies::AiState state)
	{
		switch (state)
		{
			case enemies::AiState::Idle:	return "Idle";
			case enemies::AiState::Wander:	return "Wander";
			case enemies::AiState::Chase:	return "Chase";
			case enemies::AiState::Attack:	return "Attack";
			case enemies::AiState::Dying:	return "Dying";
		}
		return "?";
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

	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("god", context.playerState->GetGodFlag(), "Invulnerable: enemies do no damage")));

	add("player_hp", "player_hp [value]", "Prints or sets the player's health (0 kills)", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		float value;
		if (args.Count() > 0 && !args.GetFloat(0, value))
		{
			con.PrintError("usage: player_hp [value]");
			return;
		}
		if (args.Count() > 0)
		{
			context.playerState->SetHealth(value);
			if (!context.playerState->IsAlive())
				eng::Engine::GetInstance().GetEvents().Publish(PlayerDiedEvent{});
		}
		con.Print("player_hp: %.0f / %.0f", static_cast<double>(context.playerState->GetHealth()), static_cast<double>(PlayerState::MAX_HEALTH));
	});

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
	// The gun's live tuning (Debug only): the defaults are GunView::Tuning's
	if (context.gunView)
	{
		GunView::Tuning& tuning = context.gunView->GetTuning();
		const auto vector3Command = [&](const char* name, Vector3* target, const char* help)
		{
			const std::string usage = std::string(name) + " [x y z]";
			add(name, usage.c_str(), help, [target, name, usage](const eng::ConsoleArgs& args, eng::DevConsole& con)
			{
				if (args.Count() > 0 && !args.GetVector3(0, *target))
				{
					con.PrintError("usage: %s", usage.c_str());
					return;
				}
				con.Print("%s = %g %g %g", name, (double)target->x, (double)target->y, (double)target->z);
			});
		};
		m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("gun_obj", &tuning.useObj, "1 = the OBJ gun, 0 = the primitive gun")));
		m_tokens.push_back(commands.RegisterVar(eng::ConVar::Float("gun_len", &tuning.length, 0.05f, 2.0f, "Length of the OBJ gun along -Z, meters")));
		vector3Command("gun_ofs", &tuning.offset, "Offset of the gun in view space (right, up, back), meters");
		vector3Command("gun_rot", &tuning.rotation, "Rotation of the OBJ model (pitch yaw roll), degrees");
		vector3Command("gun_muzzle", &tuning.muzzle, "Moves the tip flash from the barrel's end, meters");
	}
#endif

	//////////////////////////////////////////
	// Enemies
	//////////////////////////////////////////
	add("enemy_spawn", "enemy_spawn <drifter|stalker|brute> [n]", "Spawns n enemies in front of the player", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		constexpr int MAX_COUNT = 256;
		enemies::EnemyType type{};
		int count = 1;
		if (!enemies::ParseEnemyType(args.GetString(0), type) || (args.Count() > 1 && !args.GetInt(1, count)) || count < 1 || count > MAX_COUNT)
		{
			con.PrintError("usage: enemy_spawn <drifter|stalker|brute> [n]  (n = 1..%d)", MAX_COUNT);
			return;
		}

		const int spawned = SpawnInFront(context, std::vector<enemies::EnemyType>(count, type));
		if (spawned < 0)
			con.PrintError("enemy_spawn: there is no player");
		else if (spawned < count)
			con.PrintError("enemy_spawn: pool full, %d of %d spawned", spawned, count);
		else
			con.Print("enemy_spawn: %d %s spawned", spawned, enemies::GetArchetype(type).name);
	});

	add("enemy_spawn_all", "enemy_spawn_all", "Spawns the 20 enemies of the wave table at once (pool capacity check)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		// Plan §6.2, the three waves added up: 7 + 7 + 6 = 20 (15 Drifters, 4 Stalkers, 1 Brute)
		std::vector<enemies::EnemyType> types;
		types.insert(types.end(), 15, enemies::EnemyType::Drifter);
		types.insert(types.end(), 4, enemies::EnemyType::Stalker);
		types.insert(types.end(), 1, enemies::EnemyType::Brute);

		const int spawned = SpawnInFront(context, types);
		if (spawned < 0)
			con.PrintError("enemy_spawn_all: there is no player");
		else if (spawned < static_cast<int>(types.size()))
			con.PrintError("enemy_spawn_all: pool full, %d of %d spawned", spawned, static_cast<int>(types.size()));
		else
			con.Print("enemy_spawn_all: %d spawned", spawned);
	});

	add("enemy_list", "enemy_list", "Lists the enemies: id, type, state, hp, distance to the player", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		Vector3 playerPosition{};
		Vector3 playerForward{};
		const bool hasPlayer = GetPlayerPose(context, playerPosition, playerForward);

		int listed = 0;
		context.enemyManager->GetRegistry().Each<enemies::EnemyKind, enemies::Life, enemies::Brain, enemies::Transform>(
			[&](eng::ecs::Entity e, enemies::EnemyKind& kind, enemies::Life& life, enemies::Brain& brain, enemies::Transform& transform)
		{
			const float distance = hasPlayer ? Vector3Distance(playerPosition, transform.position) : 0.0f;
			con.Print("%u:%u  %-8s %-7s hp %g  %.1f m", e.index, e.generation, enemies::GetArchetype(kind.type).name,
				StateName(brain.state), static_cast<double>(life.hp), static_cast<double>(distance));
			++listed;
		});

		if (listed == 0)
			con.Print("enemy_list: no enemies");
	});

	add("kill_all", "kill_all", "Kills every enemy (each plays its death)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		const int count = context.enemyManager->GetPool().GetAliveCount();
		context.enemyManager->GetRegistry().Each<enemies::Life>([](eng::ecs::Entity, enemies::Life& life) { life.hp = 0.0f; });
		con.Print("kill_all: %d enemies removed", count);
	});

	add("wave_start", "wave_start <1-3>", "Starts a wave now (enemies that are alive stay)", [context](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		int number;
		if (!args.GetInt(0, number) || number < 1 || number > enemies::WaveDirector::WAVE_COUNT)
		{
			con.PrintError("usage: wave_start <1-%d>", enemies::WaveDirector::WAVE_COUNT);
			return;
		}
		context.enemyManager->GetWaves().StartWave(number);
		con.Print("wave_start: wave %d", number);
	});

	add("wave_skip", "wave_skip", "Kills everything of the current wave (the deaths and the next wave run as normal)", [context](const eng::ConsoleArgs&, eng::DevConsole& con)
	{
		context.enemyManager->GetWaves().SkipWave();
		con.Print("wave_skip: wave %d cleared", context.enemyManager->GetWaves().GetWaveNumber());
	});

	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("waves", context.enemyManager->GetWaves().GetEnabledFlag(),
		"Wave director on / off (off: a sandbox with only the enemies you spawn)")));

	add("enemy_tune", "enemy_tune <type> <field> <value>", "Changes an enemy type's value, for later spawns", [](const eng::ConsoleArgs& args, eng::DevConsole& con)
	{
		enemies::EnemyType type{};
		float value;
		if (!enemies::ParseEnemyType(args.GetString(0), type) || args.Count() < 3 || !args.GetFloat(2, value))
		{
			con.PrintError("usage: enemy_tune <drifter|stalker|brute> <field> <value>  (fields: %s)", enemies::GetTuneFields());
			return;
		}
		if (!enemies::TuneArchetype(type, args.GetString(1), value))
			con.PrintError("enemy_tune: unknown field '%s' or a negative value (fields: %s)", args.GetString(1).c_str(), enemies::GetTuneFields());
		else
			con.Print("enemy_tune: %s %s = %g (later spawns)", enemies::GetArchetype(type).name, args.GetString(1).c_str(), static_cast<double>(value));
	});

	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("ai_debug", &context.enemyManager->GetDebugSettings().draw,
		"Enemy AI debug: wall probes and state labels")));
	m_tokens.push_back(commands.RegisterVar(eng::ConVar::Bool("ai_freeze", &context.enemyManager->GetDebugSettings().freeze,
		"Enemies hang in the air: AI and movement skipped (aim and damage tests)")));

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
