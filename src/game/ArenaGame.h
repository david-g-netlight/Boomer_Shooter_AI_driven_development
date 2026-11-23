#pragma once
#include <eng.h>
#include "state/PlayerState.h"
#include "state/GameState.h"
#include "state/SoundFXBank.h"
#include "player/PlayerInput.h"
#include "player/PlayerActor.h"
#include "player/Gun.h"
#include "fx/ShotFXSpawner.h"
#include "flow/GameContext.h"
#include "flow/GameplayManager.h"
#include "flow/TriggerZone.h"
#include "ui/GameUI.h"
#include "ui/GunHUD.h"
#include "ui/SettingsScreen.h"
#include "state/Settings.h"
#include "player/GunView.h"
#include "audio/MusicManager.h"
#include "dev/GameCommands.h"
#include "enemies/EnemyManager.h"
#include "enemies/EnemyEvents.h"

/// <summary>
/// Boomer Shooter (placeholder name): an arena FPS. Owns the level, camera and systems of the game.
/// The Application layer: wires the engine to the game systems and owns them.
///   PlayerState / PlayerInput   the player's numbers and what the player asks for
///   GameState / GameplayManager the rules of a run and the level sequence
///   ShotFXSpawner / GameUI / GunHUD / MusicManager   what the player sees and hears
/// Levels are .lvl files (assets/levels); the player is spawned by the game at the level's spawn point.
/// </summary>
class ArenaGame : public eng::Application
{
public:
	bool		Init()							override;
	void		Update(float deltaTime)			override;
	void		DrawViewModel()					override;
	void		DrawHUD()						override;
	void		FixedUpdate(float fixedDeltaTime)	override;
	void		SubmitRenderables(eng::RenderQueue& queue)	override;
	void		Destroy()						override;

	const char*	GetTitle() const				override;
	eng::Level*	GetActiveLevel()				override;
	void		OnStartRequested()				override;
	void		OnReturnToMainMenu()			override;
	void		OnRestartRequested()			override;
	void		OnLevelLoaded()					override;
	bool		HasSettings() const				override { return true; }
	void		OnSettingsRequested()			override;
	std::string	GetLevelPath() const			override;

private:
	void		RegisterComponents();
	void		BindInput();
	bool		LoadLevelFile(const std::string& path);		// the GameplayManager's loader
	void		SpawnPlayer();
	void		UpdateMenuCamera(float deltaTime);
	void		OnPlayerHit(const enemies::PlayerHitEvent& hit);
		void		StopFootsteps();

	eng::Level		m_level;
	eng::Camera		m_camera;
	eng::ActorHandle m_player;

	PlayerState		m_playerState;
	PlayerInput		m_input;
	GameState		m_gameState;
	GameplayManager	m_gameplay;
	ShotFXSpawner	m_shotFX;
	GameUI			m_gameUI;
	GunHUD			m_gunHUD;
	GunView			m_gunView;
	MusicManager	m_music;
	enemies::EnemyManager	m_enemies;
	eng::EventBus::Token	m_playerHitToken	= 0;
	eng::EventBus::Token	m_playerDiedToken	= 0;
	eng::EventBus::Token	m_wavesClearedToken	= 0;
		eng::EventBus::Token	m_gameStartedToken	= 0;
		eng::EventBus::Token	m_gameWonToken		= 0;
#if ENG_DEV_CONSOLE
	GameCommands	m_commands;
#endif

	// Main menu backdrop: a slow orbit around the level
	static constexpr float	ORBIT_SPEED		= 10.0f;	// degrees/s
	static constexpr float	ORBIT_RADIUS	= 6.0f;		// inside the arena (16 m wide)
	static constexpr float	ORBIT_HEIGHT	= 4.0f;
	float			m_orbitAngle	= 40.0f;
};
