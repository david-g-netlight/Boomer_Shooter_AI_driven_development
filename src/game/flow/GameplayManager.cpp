#include "GameplayManager.h"
#include "flow/TriggerZone.h"
#include "state/GameState.h"
#include "state/PlayerState.h"

GameplayManager::~GameplayManager()
{
	Shutdown();
}

void GameplayManager::Init(GameState& gameState, PlayerState& playerState, LevelList levels, LevelLoader loader)
{
	Shutdown();
	m_gameState		= &gameState;
	m_playerState	= &playerState;
	m_levels		= std::move(levels);
	m_loader		= std::move(loader);
	m_nextToken		= eng::Engine::GetInstance().GetEvents().Subscribe<NextLevelRequestedEvent>(
		[this](const NextLevelRequestedEvent&) { LoadNextLevel(); });
}

void GameplayManager::Shutdown()
{
	if (m_nextToken != 0)
		eng::Engine::GetInstance().GetEvents().Unsubscribe(m_nextToken);
	m_nextToken	= 0;
	m_pending	= -1;
	m_loader	= nullptr;
}

void GameplayManager::Update()
{
	if (m_pending < 0)
		return;

	const int index = m_pending;
	m_pending = -1;
	LoadNow(index);
}

void GameplayManager::LoadLevel(int index)
{
	if (!m_levels.IsValid(index))
	{
		eng::Log::Warning("GameplayManager: no level %d (the list has %zu)", index, m_levels.GetCount());
		return;
	}
	m_pending = index;
}

bool GameplayManager::LoadNextLevel()
{
	if (!HasNextLevel())
		return false;

	LoadLevel(GetLevelIndex() + 1);
	return true;
}

void GameplayManager::ReloadCurrentLevel()
{
	LoadLevel(GetLevelIndex());
}

void GameplayManager::LoadNow(int index)
{
	if (!m_gameState || !m_playerState || !m_loader || !m_levels.IsValid(index))
		return;

	// A won level hands over to the next one through GameState (publishes LevelProgressedEvent)
	if (m_gameState->GetStatus() == GameStatus::Won && index == m_gameState->GetLevelIndex() + 1)
		m_gameState->ProgressToNextLevel();

	m_gameState->SetLevelIndex(index);
	m_gameState->Reset();
	m_playerState->Reset();

	const std::string& path = m_levels.GetPath(index);
	if (!m_loader(path))
		eng::Log::Warning("GameplayManager: '%s' could not be loaded", path.c_str());

	// The menu's Start begins level 1 through the ModeMachine hook; every other load happens while playing
	if (eng::Engine::GetInstance().GetModes().GetMode() == eng::EngineMode::Play)
		m_gameState->StartGame();

	eng::Log::Info("GameplayManager: level %d of %zu loaded", index + 1, m_levels.GetCount());
}

int GameplayManager::GetLevelIndex() const				{ return m_gameState ? m_gameState->GetLevelIndex() : 0; }
const LevelList& GameplayManager::GetLevels() const		{ return m_levels; }
std::string GameplayManager::GetCurrentPath() const		{ return m_levels.GetPath(GetLevelIndex()); }
bool GameplayManager::HasNextLevel() const				{ return m_levels.HasNext(GetLevelIndex()); }
bool GameplayManager::HasPendingRequest() const			{ return m_pending >= 0; }
