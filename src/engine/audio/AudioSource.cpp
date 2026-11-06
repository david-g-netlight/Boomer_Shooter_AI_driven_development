#include "AudioSource.h"
#include "core/Engine.h"
#include <algorithm>
#include <raymath.h>

namespace eng
{
	namespace
	{
		constexpr float MAX_PAN = 0.8f;	// never fully hard-panned, sounds more natural on headphones
	}

	AudioSource::AudioSource(const Sound* sound)
		: m_sound(sound)
	{
	}

	void AudioSource::SetSound(const Sound* sound)			{ m_sound = sound; }
	void AudioSource::SetVolume(float volume)				{ m_volume = std::clamp(volume, 0.0f, 1.0f); }
	void AudioSource::SetPitch(float pitch)					{ m_pitch = std::max(pitch, 0.01f); }
	void AudioSource::SetPitchVariation(float variation)	{ m_pitchVariation = std::max(variation, 0.0f); }
	void AudioSource::SetSpatial(bool spatial)				{ m_isSpatial = spatial; }
	void AudioSource::SetPosition(const Vector3& position)	{ m_position = position; }
	const Vector3& AudioSource::GetPosition() const			{ return m_position; }

	void AudioSource::SetRange(float minDistance, float maxDistance)
	{
		m_minDistance = std::max(minDistance, 0.0f);
		m_maxDistance = std::max(maxDistance, m_minDistance + 0.01f);
	}

	void AudioSource::Play()
	{
		if (!m_sound)
			return;

		float pitch = m_pitch;
		if (m_pitchVariation > 0.0f)
			pitch += m_pitchVariation * (static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f);

		m_voice = Engine::GetInstance().GetAudio().Play(*m_sound, m_volume * ComputeGain(), pitch, ComputePan());
	}

	void AudioSource::Stop()
	{
		Engine::GetInstance().GetAudio().Stop(m_voice);
		m_voice = {};
	}

	bool AudioSource::IsPlaying() const
	{
		return Engine::GetInstance().GetAudio().IsPlaying(m_voice);
	}

	void AudioSource::Update()
	{
		if (!m_isSpatial || !IsPlaying())
			return;

		AudioManager& audio = Engine::GetInstance().GetAudio();
		audio.SetVoiceVolume(m_voice, m_volume * ComputeGain());
		audio.SetVoicePan(m_voice, ComputePan());
	}

	float AudioSource::ComputeGain() const
	{
		if (!m_isSpatial)
			return 1.0f;

		const float distance = Vector3Distance(m_position, Engine::GetInstance().GetAudio().GetListenerPosition());
		if (distance <= m_minDistance) return 1.0f;
		if (distance >= m_maxDistance) return 0.0f;

		// Quadratic falloff: drops faster near the source, closer to how loudness is perceived
		const float t = (distance - m_minDistance) / (m_maxDistance - m_minDistance);
		return (1.0f - t) * (1.0f - t);
	}

	float AudioSource::ComputePan() const
	{
		if (!m_isSpatial)
			return 0.0f;

		const AudioManager& audio = Engine::GetInstance().GetAudio();
		const Vector3 toSource = Vector3Subtract(m_position, audio.GetListenerPosition());
		if (Vector3LengthSqr(toSource) < 1e-6f)
			return 0.0f;

		return MAX_PAN * Vector3DotProduct(Vector3Normalize(toSource), audio.GetListenerRight());
	}
}
