#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun's 2D HUD: crosshair, the scale factor and the cooldown. Drawn in screen points by Application::DrawHUD().
///   crosshair  white; green when the aim is on a Scalable prop in reach; dimmed with a ring that empties while the
///              gun cools down
///   factor     "x0.50" with one pip per step (x0.25, x0.5, x0.75), bottom center
///   hints      LMB stretch, RMB shrink, wheel factor
/// </summary>
class GunHUD
{
public:
	void	Draw(const eng::ActorHandle& player, const PlayerState& state) const;
};
