#pragma once
#include <eng.h>
#include "state/PlayerState.h"
#include "state/GameState.h"
#include "state/SoundFXBank.h"
#include "player/PlayerInput.h"
#include "player/PlayerActor.h"
#include "player/ScalingGun.h"
#include "fx/RayBeamSpawner.h"
#include "flow/GameContext.h"
#include "flow/GameplayManager.h"
#include "flow/TriggerZone.h"
#include "ui/GameUI.h"
#include "ui/GunHUD.h"
#include "ui/SettingsScreen.h"
#include "state/Settings.h"
#include "player/GunView.h"
#include "audio/MusicManager.h"

/// <summary>
/// "Build to Scale": an FPS puzzle game where the gun scales props up and down.
/// The Application layer: wires the engine to the game systems and owns them.
///   PlayerState / PlayerInput   the player's numbers and what the player asks for
///   GameState / GameplayManager the rules of a run and the level sequence
///   RayBeamSpawner / GameUI / GunHUD / MusicManager   what the player sees and hears
/// Levels are .lvl files (assets/levels); the player is spawned by the game at the level's spawn point.
/// </summary>
class ScaleGame : public eng::Application
{
public:
	bool		Init()							override;
	void		Update(float deltaTime)			override;
	void		DrawViewModel()					override;
	void		DrawHUD()						override;
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

	eng::Level		m_level;
	eng::Camera		m_camera;
	eng::ActorHandle m_player;

	PlayerState		m_playerState;
	PlayerInput		m_input{ m_playerState };
	GameState		m_gameState;
	GameplayManager	m_gameplay;
	RayBeamSpawner	m_beams;
	GameUI			m_gameUI;
	GunHUD			m_gunHUD;
	GunView			m_gunView;
	MusicManager	m_music;

	// Main menu backdrop: a slow orbit around the level
	static constexpr float	ORBIT_SPEED		= 10.0f;	// degrees/s
	static constexpr float	ORBIT_RADIUS	= 20.0f;
	float			m_orbitAngle	= 40.0f;
};
