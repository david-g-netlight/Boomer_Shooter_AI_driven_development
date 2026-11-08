#include "SettingsScreen.h"
#include <raygui.h>
#include "state/Settings.h"

void SettingsScreen::Update(float /*unscaledDeltaTime*/)
{
	if (IsKeyPressed(KEY_ESCAPE))
		Close();
}

void SettingsScreen::Draw()
{
	Settings& settings = Settings::Instance();
	const Settings before = settings;

	eng::UIStyle::DimBackground(0.7f);
	eng::UIStyle::DrawTitle("Settings", 0.12f * static_cast<float>(GetScreenHeight()));

	// A centered column: label on the left, slider on the right, value text after it
	const float width	= 560.0f;
	const float x		= 0.5f * (static_cast<float>(GetScreenWidth()) - width);
	float y				= 0.12f * static_cast<float>(GetScreenHeight()) + 90.0f;
	const float row		= 46.0f;
	const float labelW	= 190.0f;
	const float sliderW	= 260.0f;

	auto slider = [&](const char* label, float& value, float min, float max)
	{
		DrawText(label, static_cast<int>(x), static_cast<int>(y) + 6, 20, RAYWHITE);
		GuiSlider({ x + labelW, y, sliderW, 32.0f }, nullptr, nullptr, &value, min, max);
		DrawText(TextFormat("%.2f", static_cast<double>(value)), static_cast<int>(x + labelW + sliderW + 14.0f), static_cast<int>(y) + 6, 20, RAYWHITE);
		y += row;
	};

	slider("Mouse sensitivity", settings.mouseSensitivity, Settings::MIN_SENSITIVITY, Settings::MAX_SENSITIVITY);
	slider("Master volume", settings.masterVolume, 0.0f, 1.0f);
	slider("Music volume", settings.musicVolume, 0.0f, 1.0f);
	slider("Effects volume", settings.sfxVolume, 0.0f, 1.0f);

	DrawText("Fullscreen", static_cast<int>(x), static_cast<int>(y) + 6, 20, RAYWHITE);
	GuiCheckBox({ x + labelW, y + 4.0f, 24.0f, 24.0f }, nullptr, &settings.fullscreen);
	y += row + 10.0f;

	if (GuiButton(eng::UIStyle::ButtonRect(0, y), "Back"))
		Close();

	// Apply as soon as something moved (the volume slider is heard while dragging)
	if (settings.masterVolume != before.masterVolume || settings.musicVolume != before.musicVolume
		|| settings.sfxVolume != before.sfxVolume || settings.fullscreen != before.fullscreen)
		settings.Apply();
}

void SettingsScreen::OnPop()
{
	Settings::Instance().Save();
}

void SettingsScreen::Close()
{
	eng::Engine::GetInstance().GetUI().Remove(this);
}
