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
	m_tokens[0] = events.Subscribe<GameWonEvent>([this](const GameWonEvent& e)		{ ShowWon(e.level); });
	m_tokens[1] = events.Subscribe<GameLostEvent>([this](const GameLostEvent& e)	{ ShowLost(e.level); });
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

void GameUI::ShowWon(int level)
{
	if (!m_input || !m_gameplay)
		return;

	m_input->SetControl(false);
	GameplayManager* gameplay = m_gameplay;
	auto mainMenu = [] { eng::Engine::GetInstance().GetModes().Request(eng::EngineMode::MainMenu); };

	if (gameplay->HasNextLevel())
	{
		eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("Level Complete", "Level " + std::to_string(level + 1) + " cleared.",
			std::vector<eng::MessageScreen::Button>{
				{ "Next Level", [gameplay] { gameplay->LoadNextLevel(); } },
				{ "Main Menu", mainMenu } });
	}
	else
	{
		eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("You did it!", "Every level is cleared.",
			std::vector<eng::MessageScreen::Button>{
				{ "Play Again", [gameplay] { gameplay->LoadLevel(0); } },
				{ "Main Menu", mainMenu } });
	}
}

void GameUI::ShowLost(int /*level*/)
{
	if (!m_input || !m_gameplay)
		return;

	m_input->SetControl(false);
	GameplayManager* gameplay = m_gameplay;
	eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("You fell!", "Try again.",
		std::vector<eng::MessageScreen::Button>{
			{ "Retry", [gameplay] { gameplay->ReloadCurrentLevel(); } },
			{ "Main Menu", [] { eng::Engine::GetInstance().GetModes().Request(eng::EngineMode::MainMenu); } } });
}
