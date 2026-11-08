#pragma once

enum class GameStatus
{
	Idle,		// loaded, not started (menu, or between levels)
	Playing,
	Won,
	Lost,
};

/// <summary>
/// Published on the EventBus by GameState; `level` is the index in the LevelList.
/// </summary>
struct GameStartedEvent			{ int level; };
struct GameWonEvent				{ int level; };
struct GameLostEvent			{ int level; };
struct LevelProgressedEvent		{ int level; };		// the new level index

/// <summary>
/// The rules of a run (Unity GameState): Idle -> Playing -> Won | Lost. Win and lose are game rules, so they
/// live here and not in the engine's ModeMachine. Every call that is not allowed from the current status is
/// ignored and returns false, so a trigger that fires twice can't win and lose the same level.
/// </summary>
class GameState
{
public:
	/// <summary>
	/// Back to Idle (level index is kept: a retry reloads the same level).
	/// </summary>
	void		Reset();

	bool		StartGame();				// Idle -> Playing
	bool		WinGame();					// Playing -> Won
	bool		LoseGame();					// Playing -> Lost

	/// <summary>
	/// Won -> Idle on the next level index (the GameplayManager loads it and starts it).
	/// </summary>
	bool		ProgressToNextLevel();

	GameStatus	GetStatus() const;
	bool		IsPlaying() const;
	int			GetLevelIndex() const;
	void		SetLevelIndex(int index);

	static const char* StatusName(GameStatus status);

private:
	GameStatus	m_status		= GameStatus::Idle;
	int			m_levelIndex	= 0;
};
