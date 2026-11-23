#include "MusicManager.h"

MusicManager::~MusicManager()
{
	Shutdown();
}

void MusicManager::Init(eng::ResourceManager& resources)
{
	Shutdown();
	m_menu		= resources.LoadMusic(MENU_TRACK);
	m_gamePlay	= resources.LoadMusic(GAME_TRACK);
	eng::Engine::GetInstance().GetAudio().GetMusic().SetVolume(VOLUME);

	m_token = eng::Engine::GetInstance().GetEvents().Subscribe<eng::ModeChangedEvent>([this](const eng::ModeChangedEvent& e)
	{
		if (e.current == eng::EngineMode::MainMenu)		Play(Theme::Menu);
		else if (e.current == eng::EngineMode::Play)	Play(Theme::GamePlay);
	});
}

void MusicManager::Shutdown()
{
	if (m_token != 0)
		eng::Engine::GetInstance().GetEvents().Unsubscribe(m_token);
	m_token		= 0;
	m_menu		= nullptr;		// the ResourceManager owns the streams
	m_gamePlay	= nullptr;
	m_current	= Theme::None;
}

void MusicManager::Play(Theme theme)
{
	Music* track = theme == Theme::Menu ? m_menu : (theme == Theme::GamePlay ? m_gamePlay : nullptr);
	if (!track || theme == m_current)
		return;

	eng::MusicPlayer& player = eng::Engine::GetInstance().GetAudio().GetMusic();
	player.Play(*track, player.GetCurrent() ? CROSSFADE : FADE_IN);
	m_current = theme;
}

MusicManager::Theme MusicManager::GetCurrent() const	{ return m_current; }

const char* MusicManager::GetCurrentName() const
{
	switch (m_current)
	{
		case Theme::Menu:		return "menu";
		case Theme::GamePlay:	return "game";
		case Theme::None:		break;
	}
	return "none";
}
