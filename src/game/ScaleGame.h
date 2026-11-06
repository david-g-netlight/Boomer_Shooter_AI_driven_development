#pragma once
#include <eng.h>
#include <vector>

/// <summary>
/// "Build to Scale" game. Phase 1: temporary scene with an orbiting camera and a cube,
/// plus an overlay that shows the engine's time and window state.
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

private:
	void		BindInput();
	void		DrawInputPanel() const;
	void		SubmitDebugShapes() const;
	void		DrawMinimap();

	eng::Camera	m_camera;
	// Phase 3 "Done when" scene: a ring of cubes, one material each (created in Init: needs the window)
	static constexpr int		CUBE_COUNT	= 10;
	std::vector<eng::Material>	m_cubeMaterials;
	std::vector<Matrix>			m_cubeTransforms;
	std::vector<Vector3>		m_cubePositions;

	// Top-down minimap: the same RenderQueue drawn a second time with another camera
	static constexpr int		MINIMAP_WIDTH	= 240;	// screen points
	static constexpr int		MINIMAP_HEIGHT	= 160;
	eng::Camera					m_minimapCamera;
	eng::RenderTarget			m_minimap;
	float		m_orbitAngle			= 45.0f;	// degrees, used while the cursor is unlocked

	// Fixed steps counted over the last second, to show the fixed rate on screen
	int			m_fixedStepsThisSecond	= 0;
	int			m_fixedStepsPerSecond	= 0;
	float		m_secondTimer			= 0.0f;

	// Input debug: "pressed" must fire exactly once per press
	static constexpr float	LOOK_SENSITIVITY	= 0.1f;	// degrees per screen point
	static constexpr float	FLY_SPEED			= 5.0f;	// m/s, temporary fly camera
	static constexpr float	ORBIT_SPEED			= 20.0f;	// degrees/s
	static constexpr float	ORBIT_RADIUS		= 8.5f;

	int			m_jumpPresses			= 0;
	int			m_firePresses			= 0;
	int			m_altFirePresses		= 0;
	float		m_scrollTotal			= 0.0f;
};
