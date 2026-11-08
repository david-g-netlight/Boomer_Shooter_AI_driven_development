#pragma once
#include <eng.h>
#include <optional>
#include <vector>

/// <summary>
/// "Build to Scale" game. Until the gameplay phases, a test scene that exercises the engine.
/// Phase 6: a physics sandbox (static ground and ramps, 20 dynamic crates), plus the camera, audio and
/// minimap from earlier phases.
/// </summary>
class TriggerCounter;

class ScaleGame : public eng::Application
{
public:
	bool		Init()							override;
	void		Update(float deltaTime)			override;
	void		FixedUpdate(float fixedDeltaTime) override;
	void		SubmitRenderables(eng::RenderQueue& queue) override;
	void		DrawHUD()						override;
	void		Destroy()						override;

	const char*	GetTitle() const				override;
	eng::Level*	GetActiveLevel()				override;
	void		OnRestartRequested()			override;
	void		OnLevelLoaded()					override;
	std::string	GetLevelPath() const			override;

private:
	struct BoxDesc
	{
		const char*		name;
		Vector3			position;
		Vector3			size;					// full size in meters
		Vector3			rotationDegrees	= { 0.0f, 0.0f, 0.0f };
		eng::BodyType	type			= eng::BodyType::Dynamic;
		float			mass			= 1.0f;
		Color			color			= WHITE;
		uint32_t		layer			= eng::Layer::Prop;
	};

	void		BindInput();
	void		ClearSandbox();
	void		BuildSandbox();
	eng::GameActor*	SpawnBox(const BoxDesc& desc);
	void		UpdateCamera(float deltaTime);
	void		UpdatePlayer(float deltaTime);
	void		SpawnPlayer();
	void		ThrowPellet();
	void		UpdatePellets();
	void		EmitImpact(float sign);
	void		FireBeam(float sign);
	void		UpdateAudioDemo();
	void		PruneDestroyedCrates();
	void		UpdateAim();
	bool		IsCrate(const eng::GameActor* actor) const;
	void		SubmitDebugShapes() const;
	void		DrawMinimap();
	void		DrawPhysicsPanel() const;
	void		DrawAudioPanel() const;

	eng::Level	m_level;
	eng::Camera	m_camera;

	//////////////////////////////////////////
	/// Physics sandbox (Phase 6)
	//////////////////////////////////////////
	static constexpr float	HEAVY_MASS		= 20.0f;	// kg, the pyramid
	static constexpr float	LIGHT_MASS		= 2.0f;		// kg, the tower: light enough to push
	std::vector<eng::GameActor*>	m_crates;			// dynamic crates still alive
	static constexpr float	AIM_REACH		= 30.0f;	// m, Unity PlayerState ray reach
	static constexpr float	KICK_SPEED		= 6.0f;		// m/s given to the aimed crate by LMB
	std::optional<eng::RaycastHit>	m_aimHit;			// this frame's center-screen ray (valid until frame end)
	int						m_deletedCrates	= 0;
	TriggerCounter*			m_triggerCounter = nullptr;	// on the Trigger Zone actor (rebuilt on Restart)
	eng::EventBus::Token	m_triggerToken	= 0;

	// Player capsule (CharacterMotor). V toggles fly camera <-> first person
	static constexpr Vector3	PLAYER_SPAWN	= { 0.0f, 1.0f, 10.0f };
	static constexpr float		WALK_SPEED		= 6.0f;		// m/s (Unity PlayerState uses 10; tuned in Phase 10)
	static constexpr float		JUMP_SPEED		= 8.0f;		// m/s: with gravity 32 the apex is ~1 m (Unity jump arc)
	static constexpr float		EYE_HEIGHT		= 0.7f;		// above the capsule center (1.7 m above the feet)
	eng::GameActor*				m_player		= nullptr;
	bool						m_isPlayerMode	= false;

	// CCD check: T throws small fast pellets at a 5 cm wall; none should end up behind it
	static constexpr float		WALL_Z			= -9.0f;
	static constexpr float		PELLET_SIZE		= 0.12f;	// m, below CCD_AUTO_SIZE
	static constexpr float		PELLET_SPEED	= 40.0f;	// m/s = 0.8 m per fixed tick
	static constexpr size_t		MAX_PELLETS		= 30;
	std::vector<eng::GameActor*>	m_pellets;
	int							m_pelletsThrown	= 0;
	int							m_tunnelled		= 0;
	const Shader*			m_litShader		= nullptr;

	//////////////////////////////////////////
	/// Minimap (Phase 3): the same RenderQueue drawn a second time with another camera
	//////////////////////////////////////////
	static constexpr int		MINIMAP_WIDTH	= 240;	// screen points
	static constexpr int		MINIMAP_HEIGHT	= 160;
	eng::Camera					m_minimapCamera;
	eng::RenderTarget			m_minimap;

	//////////////////////////////////////////
	/// Fly / orbit camera (Phase 2-3)
	//////////////////////////////////////////
	static constexpr float	LOOK_SENSITIVITY	= 0.1f;		// degrees per screen point
	static constexpr float	FLY_SPEED			= 8.0f;		// m/s
	static constexpr float	ORBIT_SPEED			= 10.0f;	// degrees/s
	static constexpr float	ORBIT_RADIUS		= 20.0f;
	float		m_orbitAngle			= 40.0f;	// degrees, used while the cursor is unlocked

	// Fixed steps counted over the last second, to show the fixed rate on screen
	int			m_fixedStepsThisSecond	= 0;
	int			m_fixedStepsPerSecond	= 0;
	float		m_secondTimer			= 0.0f;

	//////////////////////////////////////////
	/// Audio (Phase 4)
	//////////////////////////////////////////
	// Unity ScalingAudioFX: stretch / shrink pitched by the scale factor
	static constexpr float	SCALE_FACTORS[3]	= { 0.25f, 0.5f, 0.75f };	// Unity PlayerState: rate 0.25 x 3
	int			m_scaleFactorIndex		= 0;
	const Sound*	m_sfxStretch		= nullptr;
	const Sound*	m_sfxShrink			= nullptr;

	// Music (Unity ThemeMusicManager: fade-in 0.05/s up to 0.2 = 4 s)
	static constexpr float	MUSIC_FADE_IN		= 4.0f;		// seconds
	static constexpr float	MUSIC_CROSSFADE		= 2.0f;
	Music*		m_themeMenu				= nullptr;
	Music*		m_themeGame				= nullptr;
};
