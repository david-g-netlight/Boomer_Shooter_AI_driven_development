#pragma once
#include <string>

/// <summary>
/// The player's options: mouse sensitivity, three volumes and fullscreen. One instance for the game.
/// Stored as `key=value` lines in the user's application-support folder
/// (~/Library/Application Support/BuildToScale/settings.cfg on macOS), so it survives updates of the game folder.
/// A missing or damaged file gives the defaults.
/// </summary>
class Settings
{
public:
	static constexpr float	MIN_SENSITIVITY	= 0.25f;
	static constexpr float	MAX_SENSITIVITY	= 3.0f;

	static Settings&	Instance();

	float		mouseSensitivity	= 1.0f;		// multiplier of FirstPersonLook::SENSITIVITY
	float		masterVolume		= 1.0f;		// 0..1
	float		musicVolume			= 1.0f;
	float		sfxVolume			= 1.0f;
	bool		fullscreen			= false;	// borderless window over the whole screen

	/// <summary>
	/// Reads the file; returns false (and keeps the defaults) if there is none. Values are clamped.
	/// </summary>
	bool		Load();
	bool		Save() const;

	/// <summary>
	/// Pushes the values to the engine: audio volumes and the window mode. Needs the window and audio device.
	/// </summary>
	void		Apply() const;

	void		ResetToDefaults();

	/// <summary>
	/// Where the file lives; SetPath() redirects it (tests).
	/// </summary>
	std::string	GetPath() const;
	void		SetPath(const std::string& path);

private:
	std::string	m_pathOverride;
};
