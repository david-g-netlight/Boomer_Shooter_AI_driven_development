#pragma once

/**
* ENGINE-INTERNAL: PhysicsWorld's Bullet objects. Included only by the physics .cpp files.
*/

#include "BulletConvert.h"
#include <memory>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "PhysicsWorld.h"
#include "TriggerVolume.h"

namespace eng
{
	struct PhysicsWorld::Impl
	{
		// Declaration order = creation order; members are destroyed in reverse (world first)
		std::unique_ptr<btGhostPairCallback>					ghostPairCallback;	// outlives the broadphase
		std::unique_ptr<btDefaultCollisionConfiguration>		configuration;
		std::unique_ptr<btCollisionDispatcher>					dispatcher;
		std::unique_ptr<btBroadphaseInterface>					broadphase;
		std::unique_ptr<btSequentialImpulseConstraintSolver>	solver;
		std::unique_ptr<btDiscreteDynamicsWorld>				world;

		std::vector<TriggerVolume*>								triggers;
	};

	struct TriggerVolume::Impl
	{
		std::unique_ptr<btPairCachingGhostObject> ghost;
	};
}
