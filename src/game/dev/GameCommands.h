#pragma once
#if ENG_DEV_CONSOLE

#include <eng.h>
#include <vector>

namespace enemies { class EnemyManager; }
class GameplayManager;
class GameState;
class PlayerState;

/// <summary>
/// What the game's console commands act on. All of it is owned by the game and outlives the registration.
/// </summary>
struct GameCommandContext
{
	GameplayManager*	gameplay	= nullptr;
	GameState*			gameState	= nullptr;
	PlayerState*		playerState	= nullptr;
	eng::ActorHandle*	player		= nullptr;
	eng::Level*			level		= nullptr;
	enemies::EnemyManager*	enemyManager	= nullptr;
};

/// <summary>
/// restart, level_list, level_load, level_next, win, lose, tp, tp_spawn, god, player_hp, gun_cooldown, gun_reach, sens, enemy_spawn, enemy_spawn_all, enemy_list, kill_all, ai_debug, ai_freeze, ecs_selftest (Debug).
/// The commands capture the context, so the game unregisters them in Destroy().
/// Level commands only make requests: the GameplayManager applies them in its next Update.
/// </summary>
class GameCommands
{
public:
	void	Register(const GameCommandContext& context);
	void	Unregister();

private:
	std::vector<eng::ConsoleCommands::Token>	m_tokens;
};

#endif
