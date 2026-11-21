#pragma once
#if ENG_DEV_CONSOLE

#include <eng.h>
#include <vector>

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
};

/// <summary>
/// restart, level_list, level_load, level_next, win, lose, tp, tp_spawn, god, gun_cooldown, gun_reach, sens, ecs_selftest (Debug).
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
