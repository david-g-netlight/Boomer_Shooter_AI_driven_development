#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun the player holds, drawn in its own pass (Application::DrawViewModel): a separate camera at the origin
/// looking down -Z, with the depth buffer cleared first, so it never clips into walls and ignores the
/// world camera's pitch and yaw. Its parts are primitive meshes with lit materials, drawn through a RenderQueue
/// of its own, with the world's sun turned into the view's space (the gun camera never moves).
/// It kicks back on every shot, bobs a little, its muzzle flashes, and its top light dims while the gun cools
/// down. Only drawn while playing or paused.
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
	void	SubmitPrimitiveGun(const Matrix& pose) const;
	void	SubmitBox(const eng::Material& material, const Matrix& pose, Vector3 center, Vector3 size) const;
	void	SubmitTube(const eng::Material& material, const Matrix& pose, float x, float y, float zStart, float zEnd, float radius) const;

	const PlayerState*		m_state		= nullptr;
	eng::Camera				m_camera;
	eng::EventBus::Token	m_token		= 0;
	float					m_kick		= 0.0f;		// 1 right after a shot, falls to 0
	float					m_flash		= 0.0f;		// muzzle tip glow

	// Draw() is const but fills the queue and sets the tints of the parts every frame
	struct Parts
	{
		eng::Material	barrel;
		eng::Material	tip;
		eng::Material	body;
		eng::Material	handle;
		eng::Material	light;
	};
	mutable Parts				m_parts;
	mutable eng::RenderQueue	m_queue;
};
