#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun the player holds, drawn in its own pass (Application::DrawViewModel): a separate camera at the origin
/// looking down -Z, with the depth buffer cleared first, so it never clips into walls and ignores the
/// world camera's pitch and yaw. It kicks back on every shot, bobs a little, its muzzle flashes, and its top
/// light dims while the gun cools down. Only drawn while playing or paused.
/// </summary>
class GunView
{
public:
	~GunView();

	void	Init(const PlayerState& state);
	void	Shutdown();

	void	Update(float deltaTime);
	void	Draw() const;

private:
	const PlayerState*		m_state		= nullptr;
	eng::Camera				m_camera;
	eng::EventBus::Token	m_token		= 0;
	float					m_kick		= 0.0f;		// 1 right after a shot, falls to 0
	float					m_flash		= 0.0f;		// muzzle tip glow
};
