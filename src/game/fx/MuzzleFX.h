#pragma once
#include <raylib.h>

/// <summary>
/// Sparks and a soft flash at the gun's muzzle when it fires (Unity ExitParticlesController). Particles live in
/// the engine's ParticleSystem.
/// </summary>
namespace MuzzleFX
{
	/// <summary>
	/// The shot's colour: warm muzzle flash, shared by the tracer and the impact.
	/// </summary>
	constexpr Color	START_COLOR	= { 255, 220, 120, 255 };
	constexpr Color	END_COLOR	= { 255, 110, 30, 0 };

	/// <summary>
	/// `direction` is where the shot goes.
	/// </summary>
	void Burst(const Vector3& position, const Vector3& direction);
}
