#include "TriggerZone.h"
#include "flow/GameContext.h"
#include "state/GameState.h"

void TriggerZone::OnTriggerEnter(eng::GameActor& other)
{
	const eng::Collider* collider = other.GetComponent<eng::Collider>();
	if (collider && collider->GetLayer() == eng::Layer::Player)
		OnPlayerEnter();
}

void WinTrigger::OnPlayerEnter()
{
	if (GameState* state = GameContext::GetGameState())
		state->WinGame();
}

void LoseTrigger::OnPlayerEnter()
{
	if (GameState* state = GameContext::GetGameState())
		state->LoseGame();
}

void NextLevelTrigger::OnPlayerEnter()
{
	const GameState* state = GameContext::GetGameState();
	eng::Engine::GetInstance().GetEvents().Publish(NextLevelRequestedEvent{ state ? state->GetLevelIndex() : 0 });
}
