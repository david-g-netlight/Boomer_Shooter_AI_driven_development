#pragma once
#include <eng.h>

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
	void		DrawHUD()						override;
	void		Destroy()						override;

	const char*	GetTitle() const				override;

private:
	void		BindInput();
	void		DrawInputPanel() const;

	// TODO(Phase 3): replaced by eng::Camera / eng::ActiveCamera
	Camera3D	m_camera				= {};

	// Fixed steps counted over the last second, to show the fixed rate on screen
	int			m_fixedStepsThisSecond	= 0;
	int			m_fixedStepsPerSecond	= 0;
	float		m_secondTimer			= 0.0f;

	// Input debug: "pressed" must fire exactly once per press
	static constexpr float	LOOK_SENSITIVITY	= 0.1f;	// degrees per screen point
	static constexpr float	FLY_SPEED			= 5.0f;	// m/s, temporary fly camera

	int			m_jumpPresses			= 0;
	int			m_firePresses			= 0;
	int			m_altFirePresses		= 0;
	float		m_scrollTotal			= 0.0f;
};
