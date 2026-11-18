#pragma once
#include <eng.h>

class GameplayManager;
class PlayerInput;

/// <summary>
/// The game's dialogs (Unity GUIManager). Listens to the GameState events:
///   GameWon  -> "Arena cleared" with Next Level (only when the LevelList has one, else Play Again) and Main Menu
///   GameLost -> "You died" with Retry and Main Menu
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
	void	ShowWon();
	void	ShowLost();

	PlayerInput*			m_input		= nullptr;
	GameplayManager*		m_gameplay	= nullptr;
	eng::EventBus::Token	m_tokens[3]	= {};
};
