#include "GunHUD.h"
#include <algorithm>
#include "player/Gun.h"
#include "state/PlayerState.h"

void GunHUD::Draw(const eng::ActorHandle& player, const PlayerState& state) const
{
	const int width		= GetScreenWidth();
	const int height	= GetScreenHeight();
	const int cx		= width / 2;
	const int cy		= height / 2;

	const Gun* gun				= player ? player->GetComponent<Gun>() : nullptr;
	const bool isCoolingDown	= !state.IsReadyToShoot();
	const bool onScalable		= gun && gun->GetAimHit().isTarget;

	// Crosshair
	Color color = onScalable ? Color{ 90, 255, 140, 255 } : RAYWHITE;
	if (isCoolingDown)
		color = Fade(color, 0.45f);
	const float fx = static_cast<float>(cx), fy = static_cast<float>(cy);
	DrawLineEx({ fx - 11.0f, fy }, { fx - 4.0f, fy }, 2.0f, color);
	DrawLineEx({ fx + 4.0f, fy }, { fx + 11.0f, fy }, 2.0f, color);
	DrawLineEx({ fx, fy - 11.0f }, { fx, fy - 4.0f }, 2.0f, color);
	DrawLineEx({ fx, fy + 4.0f }, { fx, fy + 11.0f }, 2.0f, color);
	DrawCircle(cx, cy, 1.5f, color);

	// Cooldown ring: full right after a shot, empties clockwise until the gun is ready
	if (isCoolingDown)
	{
		const float left = state.GetCooldownFraction();
		DrawRing({ static_cast<float>(cx), static_cast<float>(cy) }, 16.0f, 18.0f, -90.0f, -90.0f + 360.0f * left, 32, Fade(RAYWHITE, 0.8f));
	}
}
