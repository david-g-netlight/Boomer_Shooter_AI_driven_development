#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun's 2D HUD: crosshair and cooldown. Drawn in screen points by Application::DrawHUD().
///   crosshair  white; red when the aim is on a target in reach (Gun::GetAimHit: a prop or an enemy);
///              dimmed, with a ring that empties, while the gun cools down
///   health     a bar at the bottom left, and a red edge flash for 0.2 s after a hit
/// </summary>
class GunHUD
{
public:
	void	Draw(const eng::ActorHandle& player, const PlayerState& state) const;
};
