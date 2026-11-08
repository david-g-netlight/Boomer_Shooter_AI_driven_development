#include "GameState.h"
#include <eng.h>

void GameState::Reset()
{
	m_status = GameStatus::Idle;
}

bool GameState::StartGame()
{
	if (m_status != GameStatus::Idle)
		return false;

	m_status = GameStatus::Playing;
	eng::Engine::GetInstance().GetEvents().Publish(GameStartedEvent{ m_levelIndex });
	return true;
}

bool GameState::WinGame()
{
	if (m_status != GameStatus::Playing)
		return false;

	m_status = GameStatus::Won;
	eng::Engine::GetInstance().GetEvents().Publish(GameWonEvent{ m_levelIndex });
	return true;
}

bool GameState::LoseGame()
{
	if (m_status != GameStatus::Playing)
		return false;

	m_status = GameStatus::Lost;
	eng::Engine::GetInstance().GetEvents().Publish(GameLostEvent{ m_levelIndex });
	return true;
}

bool GameState::ProgressToNextLevel()
{
	if (m_status != GameStatus::Won)
		return false;

	++m_levelIndex;
	m_status = GameStatus::Idle;
	eng::Engine::GetInstance().GetEvents().Publish(LevelProgressedEvent{ m_levelIndex });
	return true;
}

GameStatus GameState::GetStatus() const		{ return m_status; }
bool GameState::IsPlaying() const			{ return m_status == GameStatus::Playing; }
int GameState::GetLevelIndex() const		{ return m_levelIndex; }
void GameState::SetLevelIndex(int index)	{ m_levelIndex = index; }

const char* GameState::StatusName(GameStatus status)
{
	switch (status)
	{
		case GameStatus::Idle:		return "Idle";
		case GameStatus::Playing:	return "Playing";
		case GameStatus::Won:		return "Won";
		case GameStatus::Lost:		return "Lost";
	}
	return "?";
}
