#pragma once
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// World conventions shared by rendering and physics.
	/// Right-handed, Y-up, meters (same as raylib/OpenGL and Bullet): +X right, +Y up, -Z forward.
	/// </summary>
	struct World
	{
		Vector3	worldUp			= { 0.0f, 1.0f,  0.0f };
		Vector3	worldRight		= { 1.0f, 0.0f,  0.0f };
		Vector3	worldForward	= { 0.0f, 0.0f, -1.0f };
		Vector3	gravity			= { 0.0f, -9.81f, 0.0f };
		Color	skyColor		= { 135, 190, 235, 255 };

		// Lighting for shaders that declare these uniforms (assets/shaders/lit.fs)
		Vector3	sunDirection	= { -0.4f, -1.0f, -0.3f };	// direction the light travels
		Vector3	sunColor		= { 1.0f, 0.96f, 0.9f };
		Vector3	ambientColor	= { 0.35f, 0.4f, 0.5f };
	};
}
