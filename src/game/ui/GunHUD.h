#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun's 2D HUD: crosshair and cooldown. Drawn in screen points by Application::DrawHUD().
///   crosshair  white; red when the aim is on a target in reach (Gun::GetAimHit, a prop until there are enemies);
///              dimmed, with a ring that empties, while the gun cools down
/// </summary>
class GunHUD
{
public:
	void	Draw(const eng::ActorHandle& player, const PlayerState& state) const;
};
