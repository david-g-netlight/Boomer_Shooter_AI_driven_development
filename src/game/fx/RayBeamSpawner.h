#pragma once
#include <eng.h>

struct ShotFiredEvent;

/// <summary>
/// Turns every ShotFiredEvent into what the player sees and hears (Unity RayBeamProvider): the muzzle burst,
/// a RayBeam actor from the muzzle to the hit point, the impact burst where it hit, and the beam sound.
/// Subscribes to the EventBus in Init(); the shot is delivered at the end of the frame it was fired in.
/// </summary>
class RayBeamSpawner
{
public:
	~RayBeamSpawner();

	/// <summary>
	/// Beams are created in `level` (which must outlive the spawner, or Shutdown() first).
	/// </summary>
	void	Init(eng::Level* level);
	void	Shutdown();

	void	Spawn(const ShotFiredEvent& shot);

private:
	eng::Level*				m_level	= nullptr;
	eng::EventBus::Token	m_token	= 0;
};
