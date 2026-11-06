#pragma once
#include <eng.h>
#include <vector>

/// <summary>
/// "Build to Scale" game. Until the gameplay phases, a test scene that exercises the engine:
/// a Level with a rotating ring of cube actors (Phase 5), materials and a minimap (Phase 3),
/// input (Phase 2) and audio (Phase 4) panels.
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
	void		BindInput();
	void		BuildScene();
	void		BuildRing();
	void		PruneDestroyedCubes();
	void		UpdateSceneDemo();
	void		UpdateAudioDemo();
	void		SubmitDebugShapes() const;
	void		DrawMinimap();
	void		DrawInputPanel() const;
	void		DrawAudioPanel() const;
	void		DrawScenePanel() const;

	eng::Camera	m_camera;

	//////////////////////////////////////////
	/// Scene (Phase 5): Ground, and a Ring actor (Rotator) whose children are the cubes.
	/// Cube 0 spins too and carries a Moon child: a 3-level hierarchy.
	//////////////////////////////////////////
	static constexpr int	CUBE_COUNT		= 10;
	static constexpr float	RING_RADIUS		= 4.0f;
	static constexpr float	RING_SPEED		= 15.0f;	// degrees/s

	eng::Level				m_level;
	eng::GameActor*			m_ring			= nullptr;
	eng::GameActor*			m_pulseCube		= nullptr;	// cube 0, pulses its "emissive" parameter
	std::vector<eng::GameActor*>	m_cubes;			// alive cubes (MeshRenderer + AudioSource); destroyed ones are pruned every frame
	const Shader*			m_litShader		= nullptr;
	const Sound*			m_ping			= nullptr;
	int						m_ringBuilds	= 0;
	float					m_pulseTime		= 0.0f;

	//////////////////////////////////////////
	/// Minimap (Phase 3): the same RenderQueue drawn a second time with another camera
	//////////////////////////////////////////
	static constexpr int		MINIMAP_WIDTH	= 240;	// screen points
	static constexpr int		MINIMAP_HEIGHT	= 160;
	eng::Camera					m_minimapCamera;
	eng::RenderTarget			m_minimap;

	//////////////////////////////////////////
	/// Camera and input (Phase 2)
	//////////////////////////////////////////
	static constexpr float	LOOK_SENSITIVITY	= 0.1f;		// degrees per screen point
	static constexpr float	FLY_SPEED			= 5.0f;		// m/s, temporary fly camera
	static constexpr float	ORBIT_SPEED			= 20.0f;	// degrees/s
	static constexpr float	ORBIT_RADIUS		= 8.5f;
	float		m_orbitAngle			= 45.0f;	// degrees, used while the cursor is unlocked

	// Input debug: "pressed" must fire exactly once per press
	int			m_jumpPresses			= 0;
	int			m_firePresses			= 0;
	int			m_altFirePresses		= 0;
	float		m_scrollTotal			= 0.0f;

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
	float		m_lastPitch				= 0.0f;
	int			m_lastPingedCube		= -1;		// index into m_cubes, reset when cubes are pruned
	bool		m_isAudioPaused			= false;

	// Music (Unity ThemeMusicManager: fade-in 0.05/s up to 0.2 = 4 s)
	static constexpr float	MUSIC_FADE_IN		= 4.0f;		// seconds
	static constexpr float	MUSIC_CROSSFADE		= 2.0f;
	Music*		m_themeMenu				= nullptr;
	Music*		m_themeGame				= nullptr;
};
