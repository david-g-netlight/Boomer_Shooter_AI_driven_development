#pragma once

/**
* ENGINE-INTERNAL: PhysicsWorld's Bullet objects. Included only by the physics .cpp files.
*/

#include "BulletConvert.h"
#include <memory>
#include <btBulletDynamicsCommon.h>
#include "PhysicsWorld.h"

namespace eng
{
	struct PhysicsWorld::Impl
	{
		// Declaration order = creation order; members are destroyed in reverse (world first)
		std::unique_ptr<btDefaultCollisionConfiguration>		configuration;
		std::unique_ptr<btCollisionDispatcher>					dispatcher;
		std::unique_ptr<btBroadphaseInterface>					broadphase;
		std::unique_ptr<btSequentialImpulseConstraintSolver>	solver;
		std::unique_ptr<btDiscreteDynamicsWorld>				world;
	};
}
