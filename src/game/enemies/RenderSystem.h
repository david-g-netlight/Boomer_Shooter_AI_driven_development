#pragma once
#include <eng.h>
#include <array>
#include "EnemyTypes.h"

namespace enemies
{
	/// <summary>
	/// The 9 materials of the three types (plan §8.2): body, body while flashing, spike. Made once in
	/// EnemyManager::Init, because RenderQueue takes material pointers and each enemy must not get its own.
	/// </summary>
	struct EnemyMaterials
	{
		struct Set
		{
			eng::Material	body;
			eng::Material	bodyFlash;
			eng::Material	spike;
		};

		std::array<Set, static_cast<size_t>(EnemyType::Count)> byType;
	};

	/// <summary>
	/// Two draws per enemy: the sphere (body) and the pyramid (spike, tip along forward). The body uses the flash
	/// material and squashes while Life::hitFlash is above 0. Writes only the queue.
	/// </summary>
	void RenderSystem(eng::ecs::Registry& registry, const EnemyMaterials& materials, eng::RenderQueue& queue);
}
