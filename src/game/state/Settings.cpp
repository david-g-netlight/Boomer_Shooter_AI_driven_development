#include "Settings.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <eng.h>

Settings& Settings::Instance()
{
	static Settings settings;
	return settings;
}

std::string Settings::GetPath() const
{
	if (!m_pathOverride.empty())
		return m_pathOverride;

	const char* home = std::getenv("HOME");
	if (!home || !*home)
		return "settings.cfg";		// next to the executable (the engine changes into its folder)
	return std::string(home) + "/Library/Application Support/BuildToScale/settings.cfg";
}

void Settings::SetPath(const std::string& path)	{ m_pathOverride = path; }

void Settings::ResetToDefaults()
{
	const std::string path = m_pathOverride;
	*this = Settings();
	m_pathOverride = path;
}

bool Settings::Load()
{
	std::ifstream in(GetPath());
	if (!in)
		return false;

	for (std::string line; std::getline(in, line);)
	{
		const size_t equals = line.find('=');
		if (equals == std::string::npos)
			continue;

		const std::string key = line.substr(0, equals);
		const float value = std::strtof(line.c_str() + equals + 1, nullptr);
		if (key == "mouseSensitivity")		mouseSensitivity = std::clamp(value, MIN_SENSITIVITY, MAX_SENSITIVITY);
		else if (key == "masterVolume")		masterVolume = std::clamp(value, 0.0f, 1.0f);
		else if (key == "musicVolume")		musicVolume = std::clamp(value, 0.0f, 1.0f);
		else if (key == "sfxVolume")		sfxVolume = std::clamp(value, 0.0f, 1.0f);
		else if (key == "fullscreen")		fullscreen = value != 0.0f;
	}
	return true;
}

bool Settings::Save() const
{
	const std::filesystem::path file(GetPath());
	std::error_code error;
	if (file.has_parent_path())
		std::filesystem::create_directories(file.parent_path(), error);

	std::ofstream out(file, std::ios::trunc);
	if (!out)
	{
		eng::Log::Warning("Settings: cannot write '%s'", file.string().c_str());
		return false;
	}

	out << "mouseSensitivity=" << mouseSensitivity << "\n"
		<< "masterVolume=" << masterVolume << "\n"
		<< "musicVolume=" << musicVolume << "\n"
		<< "sfxVolume=" << sfxVolume << "\n"
		<< "fullscreen=" << (fullscreen ? 1 : 0) << "\n";
	return out.good();
}

void Settings::Apply() const
{
	eng::AudioManager& audio = eng::Engine::GetInstance().GetAudio();
	audio.SetMasterVolume(masterVolume);
	audio.SetBusVolume(eng::AudioBus::Music, musicVolume);
	audio.SetBusVolume(eng::AudioBus::Sfx, sfxVolume);

	if (IsWindowReady() && fullscreen != IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))
		ToggleBorderlessWindowed();
}
