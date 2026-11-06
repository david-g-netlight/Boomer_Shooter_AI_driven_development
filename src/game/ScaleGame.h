#pragma once
#include <eng.h>
#include <vector>

/// <summary>
/// "Build to Scale" game. Until the gameplay phases, a test scene that exercises the engine.
/// Phase 6: a physics sandbox (static ground and ramps, 20 dynamic crates), plus the camera, audio and
/// minimap from earlier phases.
/// </summary>
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
	void		BuildSandbox();
	eng::GameActor*	SpawnBox(const BoxDesc& desc);
	void		UpdateCamera(float deltaTime);
	void		UpdateAudioDemo();
	void		PruneDestroyedCrates();
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
	bool		m_isAudioPaused			= false;

	// Music (Unity ThemeMusicManager: fade-in 0.05/s up to 0.2 = 4 s)
	static constexpr float	MUSIC_FADE_IN		= 4.0f;		// seconds
	static constexpr float	MUSIC_CROSSFADE		= 2.0f;
	Music*		m_themeMenu				= nullptr;
	Music*		m_themeGame				= nullptr;
};
