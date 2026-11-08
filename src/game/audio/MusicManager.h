#pragma once
#include <eng.h>

/// <summary>
/// The game's two themes (Unity ThemeMusicManager): MENU in the main menu and GAME_PLAY while playing.
/// It follows the engine's ModeChangedEvent: MainMenu -> menu theme, Play -> gameplay theme (a track that is
/// already playing is left alone, so resuming from pause or the editor doesn't restart it). Paused and Editor
/// keep the current track; the ModeMachine ducks it. The first track fades in slowly, later ones cross-fade.
/// Needs the engine to have started the mode machine after Init(), which publishes the first event.
/// </summary>
class MusicManager
{
public:
	enum class Theme { None, Menu, GamePlay };

	static constexpr float	FADE_IN			= 4.0f;		// seconds, first track
	static constexpr float	CROSSFADE		= 2.0f;		// seconds, theme to theme
	static constexpr float	VOLUME			= 0.7f;

	~MusicManager();

	void		Init(eng::ResourceManager& resources);
	void		Shutdown();

	void		Play(Theme theme);
	Theme		GetCurrent() const;
	const char*	GetCurrentName() const;

private:
	Music*					m_menu		= nullptr;
	Music*					m_gamePlay	= nullptr;
	Theme					m_current	= Theme::None;
	eng::EventBus::Token	m_token		= 0;
};
