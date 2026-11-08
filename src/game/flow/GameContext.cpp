#include "GameContext.h"

namespace
{
	GameState* s_gameState = nullptr;
}

void GameContext::SetGameState(GameState* state)	{ s_gameState = state; }
GameState* GameContext::GetGameState()				{ return s_gameState; }
