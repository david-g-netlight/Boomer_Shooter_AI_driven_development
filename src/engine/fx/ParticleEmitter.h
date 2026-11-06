#pragma once
#include "level/Component.h"
#include "fx/ParticleSystem.h"

namespace eng
{
	/// <summary>
	/// A particle burst attached to an actor: Emit() spawns the settings at the actor's world position,
	/// along its forward axis (-Z) unless a direction is given. Particles live in the Engine's
	/// ParticleSystem, so they outlive the actor.
	/// </summary>
	class ParticleEmitter : public Component
	{
	public:
		ParticleEmitter() = default;
		explicit ParticleEmitter(const ParticleEmitterSettings& settings);

		void							SetSettings(const ParticleEmitterSettings& settings);
		ParticleEmitterSettings&		GetSettings();
		const ParticleEmitterSettings&	GetSettings() const;

		void		Emit();
		void		Emit(const Vector3& direction);

		const char*	TypeName() const override { return "ParticleEmitter"; }

	private:
		ParticleEmitterSettings m_settings;
	};
}
