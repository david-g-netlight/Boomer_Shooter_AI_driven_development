#include "GunHUD.h"
#include <algorithm>
#include <cmath>
#include "player/Gun.h"
#include "state/PlayerState.h"
#include "enemies/WaveDirector.h"

void GunHUD::Draw(const eng::ActorHandle& player, const PlayerState& state, const enemies::WaveDirector& waves) const
{
	const int width		= GetScreenWidth();
	const int height	= GetScreenHeight();
	const int cx		= width / 2;
	const int cy		= height / 2;

	const Gun* gun				= player ? player->GetComponent<Gun>() : nullptr;
	const bool isCoolingDown	= !state.IsReadyToShoot();
	const bool onTarget			= gun && gun->GetAimHit().isTarget;

	// Hurt flash: red bands along the screen edges, fading out
	const float hurt = state.GetHurtFlash();
	if (hurt > 0.0f)
	{
		const Color red = Fade(Color{ 220, 20, 20, 255 }, 0.55f * hurt);
		const int band = height / 10;
		DrawRectangle(0, 0, width, band, red);
		DrawRectangle(0, height - band, width, band, red);
		DrawRectangle(0, band, band, height - 2 * band, red);
		DrawRectangle(width - band, band, band, height - 2 * band, red);
	}

	// Wave counter
	if (waves.IsEnabled())
	{
		const char* text = waves.GetState() == enemies::WaveDirector::State::Cleared
			? "Arena cleared"
			: waves.GetState() == enemies::WaveDirector::State::Waiting
				? TextFormat("Wave %d / %d - get ready", waves.GetWaveNumber(), enemies::WaveDirector::WAVE_COUNT)
				: TextFormat("Wave %d / %d - %d left", waves.GetWaveNumber(), enemies::WaveDirector::WAVE_COUNT, waves.GetEnemiesLeft());
		constexpr int FONT = 24;
		DrawText(text, (width - MeasureText(text, FONT)) / 2, 20, FONT, Fade(RAYWHITE, 0.9f));
	}

	// Health bar
	{
		constexpr int BAR_WIDTH = 220, BAR_HEIGHT = 18, MARGIN = 24;
		const float fraction = state.GetHealth() / PlayerState::MAX_HEALTH;
		const int x = MARGIN, y = height - MARGIN - BAR_HEIGHT;
		DrawRectangle(x - 2, y - 2, BAR_WIDTH + 4, BAR_HEIGHT + 4, Fade(BLACK, 0.6f));
		DrawRectangle(x, y, static_cast<int>(BAR_WIDTH * fraction), BAR_HEIGHT, fraction > 0.3f ? Color{ 70, 200, 90, 255 } : Color{ 230, 60, 50, 255 });
		DrawText(TextFormat("%d", static_cast<int>(std::ceil(state.GetHealth()))), x + BAR_WIDTH + 10, y, BAR_HEIGHT, RAYWHITE);
	}

	// Crosshair
	Color color = onTarget ? Color{ 255, 70, 60, 255 } : RAYWHITE;
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
