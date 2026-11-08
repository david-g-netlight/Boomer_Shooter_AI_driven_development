#include "GunHUD.h"
#include <algorithm>
#include "player/ScalingGun.h"
#include "state/PlayerState.h"

void GunHUD::Draw(const eng::ActorHandle& player, const PlayerState& state) const
{
	const int width		= GetScreenWidth();
	const int height	= GetScreenHeight();
	const int cx		= width / 2;
	const int cy		= height / 2;

	const ScalingGun* gun		= player ? player->GetComponent<ScalingGun>() : nullptr;
	const bool isCoolingDown	= !state.IsReadyToShoot();
	const bool onScalable		= gun && gun->IsAimingAtScalable();

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

	// Factor: text and one pip per step, bottom center
	const int baseY = height - 70;
	const char* text = TextFormat("x%.2f", static_cast<double>(state.GetFactor()));
	const int textWidth = MeasureText(text, 28);
	DrawText(text, cx - textWidth / 2 + 1, baseY + 1, 28, Fade(BLACK, 0.6f));
	DrawText(text, cx - textWidth / 2, baseY, 28, RAYWHITE);

	for (int i = 0; i < PlayerState::FACTOR_COUNT; ++i)
	{
		const int x = cx + (i - PlayerState::FACTOR_COUNT / 2) * 22;
		const bool isOn = i <= state.GetFactorIndex();
		DrawRectangle(x - 8, baseY + 36, 16, 6, isOn ? Color{ 90, 190, 255, 255 } : Fade(RAYWHITE, 0.3f));
	}

	const char* hints = "LMB stretch    RMB shrink    wheel: factor";
	DrawText(hints, cx - MeasureText(hints, 14) / 2, baseY + 50, 14, Fade(RAYWHITE, 0.75f));
}
