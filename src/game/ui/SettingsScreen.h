#pragma once
#include <eng.h>

/// <summary>
/// The options dialog (opened from the main menu and the pause menu): mouse sensitivity, master / music / sound
/// effect volumes and fullscreen. Changes apply right away; Back (or ESC) saves them.
/// </summary>
class SettingsScreen : public eng::UIScreen
{
public:
	void		Update(float unscaledDeltaTime) override;
	void		Draw() override;
	void		OnPop() override;
	bool		IsOpaque() const override { return true; }
	const char*	GetName() const override { return "Settings"; }

private:
	void		Close();
};
