#pragma once

class GameState;

/// <summary>
/// The few game objects that components created by the level loader (no constructor arguments) must reach:
/// the triggers call the GameState. Set by the game in Init(), cleared in Destroy().
/// </summary>
namespace GameContext
{
	void		SetGameState(GameState* state);
	GameState*	GetGameState();		// nullptr outside a running game
}
