#pragma once
#include <functional>
#include <string>
#include <eng.h>
#include "state/LevelList.h"

class GameState;
class PlayerState;

/// <summary>
/// Level sequence (Unity GamePlayManager): loads a level of the LevelList, the next one, or reloads the current
/// one. Every load resets PlayerState and GameState; while the engine is in Play (everything but the first
/// start from the menu, which the ModeMachine hook handles) the game starts automatically.
///
/// Loads are requests: they run in Update(), between the fixed loop and the frame's end, never inside a physics
/// callback or a UI button, so the level can be cleared safely. A NextLevelRequestedEvent asks for the next level.
/// </summary>
class GameplayManager
{
public:
	/// <summary>
	/// Builds the level at `path` (clear, load, rebind the game's actors, spawn the player); false if the file
	/// could not be used.
	/// </summary>
	using LevelLoader = std::function<bool(const std::string& path)>;

	~GameplayManager();

	void	Init(GameState& gameState, PlayerState& playerState, LevelList levels, LevelLoader loader);
	void	Shutdown();

	/// <summary>
	/// Applies the pending request. Call once per frame from Application::Update().
	/// </summary>
	void	Update();

	//////////////////////////////////////////
	/// Requests (applied by the next Update)
	//////////////////////////////////////////
	void	LoadLevel(int index);
	bool	LoadNextLevel();			// false on the last level
	void	ReloadCurrentLevel();

	/// <summary>
	/// Loads now (the game's Init, tests). Same rules as the requests.
	/// </summary>
	void	LoadNow(int index);

	int					GetLevelIndex() const;
	const LevelList&	GetLevels() const;
	std::string			GetCurrentPath() const;
	bool				HasNextLevel() const;
	bool				HasPendingRequest() const;

private:
	GameState*				m_gameState		= nullptr;
	PlayerState*			m_playerState	= nullptr;
	LevelList				m_levels;
	LevelLoader				m_loader;
	int						m_pending		= -1;	// level index to load, -1 none
	eng::EventBus::Token	m_nextToken		= 0;
};
