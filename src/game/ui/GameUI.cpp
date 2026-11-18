#include "GameUI.h"
#include "flow/GameplayManager.h"
#include "player/PlayerInput.h"
#include "state/GameState.h"

GameUI::~GameUI()
{
	Shutdown();
}

void GameUI::Init(PlayerInput& input, GameplayManager& gameplay)
{
	Shutdown();
	m_input		= &input;
	m_gameplay	= &gameplay;

	eng::EventBus& events = eng::Engine::GetInstance().GetEvents();
	m_tokens[0] = events.Subscribe<GameWonEvent>([this](const GameWonEvent&)		{ ShowWon(); });
	m_tokens[1] = events.Subscribe<GameLostEvent>([this](const GameLostEvent&)	{ ShowLost(); });
	m_tokens[2] = events.Subscribe<GameStartedEvent>([this](const GameStartedEvent&){ if (m_input) m_input->SetControl(true); });
}

void GameUI::Shutdown()
{
	eng::EventBus& events = eng::Engine::GetInstance().GetEvents();
	for (eng::EventBus::Token& token : m_tokens)
	{
		if (token != 0)
			events.Unsubscribe(token);
		token = 0;
	}
	m_input		= nullptr;
	m_gameplay	= nullptr;
}

void GameUI::ShowWon()
{
	if (!m_input || !m_gameplay)
		return;

	m_input->SetControl(false);
	GameplayManager* gameplay = m_gameplay;
	auto mainMenu = [] { eng::Engine::GetInstance().GetModes().Request(eng::EngineMode::MainMenu); };

	// Next Level only when the LevelList has one; the last level offers Play Again
	std::vector<eng::MessageScreen::Button> buttons;
	if (gameplay->HasNextLevel())
		buttons.push_back({ "Next Level", [gameplay] { gameplay->LoadNextLevel(); } });
	else
		buttons.push_back({ "Play Again", [gameplay] { gameplay->LoadLevel(0); } });
	buttons.push_back({ "Main Menu", mainMenu });

	eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("Arena cleared", "Placeholder text: waves come later.", buttons);
}

void GameUI::ShowLost()
{
	if (!m_input || !m_gameplay)
		return;

	m_input->SetControl(false);
	GameplayManager* gameplay = m_gameplay;
	eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("You died", "Placeholder text: try again.",
		std::vector<eng::MessageScreen::Button>{
			{ "Retry", [gameplay] { gameplay->ReloadCurrentLevel(); } },
			{ "Main Menu", [] { eng::Engine::GetInstance().GetModes().Request(eng::EngineMode::MainMenu); } } });
}
