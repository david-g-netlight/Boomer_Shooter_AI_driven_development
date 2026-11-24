#pragma once
#include <eng.h>
#include <array>
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>
	/// The materials of the three types: body, body while flashing, spike. Made once in EnemyManager::Init, because
	/// RenderQueue takes material pointers and each enemy must not get its own. A type with a model draws the model
	/// with body / bodyFlash (its texture; the flash is the same texture unlit); without one, the sphere and pyramid.
	/// </summary>
	struct EnemyMaterials
	{
		struct Set
		{
			eng::Material	body;
			eng::Material	bodyFlash;
			eng::Material	spike;
			const Model*	model	= nullptr;		// owned by the ResourceManager; nullptr = the sphere and pyramid
			Matrix			fit		= {};			// FitModel(bounds, Largest, 2 x bodyRadius) · RotateY(modelYawDeg)
		};

		std::array<Set, static_cast<size_t>(EnemyType::Count)> byType;
	};

	/// <summary>
	/// Two draws per enemy: the sphere (body) and the pyramid (spike, tip along forward). A type with a model draws the
	/// model in place of both. The body uses the flash material and squashes while Life::hitFlash is above 0.
	/// Writes only the queue.
	/// </summary>
	void RenderSystem(eng::ecs::Registry& registry, const EnemyMaterials& materials, eng::RenderQueue& queue);
}
