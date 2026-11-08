#pragma once
#include <raylib.h>

/// <summary>
/// Sparks and a soft wave at the gun's muzzle when it fires (Unity ExitParticlesController), tinted by the
/// shot: stretch blue-green, shrink yellow-red. Particles live in the engine's ParticleSystem.
/// </summary>
namespace MuzzleFX
{
	/// <summary>
	/// sign > 0 stretch, < 0 shrink. `direction` is where the beam goes.
	/// </summary>
	void Burst(float sign, const Vector3& position, const Vector3& direction);
}
