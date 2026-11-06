#pragma once
#include <raylib.h>
#include "AudioManager.h"
#include "level/Component.h"

namespace eng
{
	/// <summary>
	/// A sound with its own volume / pitch settings, optionally placed in the world.
	/// Spatial: volume falls off between minDistance and maxDistance from the listener (the active camera)
	/// and pan follows the direction to the source. Plays through the Engine's AudioManager.
	///
	/// As a component, the position follows its actor's world position every Update (and on Play), and the
	/// sound stops when the component is disabled or destroyed. It can also be used standalone (no actor):
	/// then call SetPosition() and RefreshSpatial() yourself.
	/// </summary>
	class AudioSource : public Component
	{
	public:
		AudioSource() = default;
		explicit AudioSource(const Sound* sound);

		void		SetSound(const Sound* sound);			// not owned (ResourceManager)
		void		SetVolume(float volume);
		void		SetPitch(float pitch);
		void		SetPitchVariation(float variation);	// each Play() picks pitch +- variation

		void		SetSpatial(bool spatial);
		void		SetPosition(const Vector3& position);	// standalone only; attached sources follow their actor
		void		SetRange(float minDistance, float maxDistance);
		const Vector3& GetPosition() const;

		void		Play();
		void		Stop();
		bool		IsPlaying() const;

		/// <summary>
		/// Re-applies distance volume and pan to the playing voice. Called by Update() when attached;
		/// standalone sources call it every frame while they move.
		/// </summary>
		void		RefreshSpatial();

		//////////////////////////////////////////
		/// Component
		//////////////////////////////////////////
		void		Update(float deltaTime) override;
		void		OnDisable() override;
		void		OnDestroy() override;
		const char*	TypeName() const override { return "AudioSource"; }

		/// <summary>
		/// Spatial gain (0..1) and pan (-1..+1) for the current listener, for debug display.
		/// </summary>
		float		ComputeGain() const;
		float		ComputePan() const;

	private:
		void			FollowOwner();

		const Sound*	m_sound				= nullptr;
		float			m_volume			= 1.0f;
		float			m_pitch				= 1.0f;
		float			m_pitchVariation	= 0.0f;

		bool			m_isSpatial			= false;
		Vector3			m_position			= { 0.0f, 0.0f, 0.0f };
		float			m_minDistance		= 2.0f;
		float			m_maxDistance		= 30.0f;

		VoiceHandle		m_voice;
	};
}
