#pragma once
#include <eng.h>

class GameplayManager;
class PlayerInput;

/// <summary>
/// The game's dialogs (Unity GUIManager). Listens to the GameState events:
///   GameWon  -> "Level Complete" with Next Level (or Play Again after the last one) and Main Menu
///   GameLost -> "You fell!" with Retry and Main Menu
/// While a dialog is up the player has no control (PlayerInput::SetControl(false)) and the cursor is free; the
/// next GameStarted gives control back. Buttons only send requests to the GameplayManager / ModeMachine.
/// </summary>
class GameUI
{
public:
	~GameUI();

	void	Init(PlayerInput& input, GameplayManager& gameplay);
	void	Shutdown();

private:
	void	ShowWon(int level);
	void	ShowLost(int level);

	PlayerInput*			m_input		= nullptr;
	GameplayManager*		m_gameplay	= nullptr;
	eng::EventBus::Token	m_tokens[3]	= {};
};
