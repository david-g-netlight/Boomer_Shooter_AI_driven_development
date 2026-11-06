#include "PhysicsWorldImpl.h"
#include "core/Log.h"
#include "physics/Collider.h"
#include "level/GameActor.h"
#include <raymath.h>

namespace eng
{
	PhysicsWorld::PhysicsWorld() = default;

	PhysicsWorld::~PhysicsWorld()
	{
		Shutdown();
	}

	bool PhysicsWorld::Init(const Vector3& gravity)
	{
		if (IsReady())
			return true;

		m_impl					= std::make_unique<Impl>();
		m_impl->configuration	= std::make_unique<btDefaultCollisionConfiguration>();
		m_impl->dispatcher		= std::make_unique<btCollisionDispatcher>(m_impl->configuration.get());
		m_impl->broadphase		= std::make_unique<btDbvtBroadphase>();
		m_impl->solver			= std::make_unique<btSequentialImpulseConstraintSolver>();
		m_impl->world			= std::make_unique<btDiscreteDynamicsWorld>(
			m_impl->dispatcher.get(), m_impl->broadphase.get(), m_impl->solver.get(), m_impl->configuration.get());

		m_impl->world->setGravity(bt::ToBt(gravity));
		Log::Info("PhysicsWorld: Bullet %d.%02d ready, gravity (%.2f, %.2f, %.2f)",
			btGetVersion() / 100, btGetVersion() % 100, gravity.x, gravity.y, gravity.z);
		return true;
	}

	void PhysicsWorld::Shutdown()
	{
		if (!m_impl)
			return;

		// Components own their bodies and should have removed them already (Level cleared first).
		// Detach anything left so the world is never deleted while objects still point into it.
		btDiscreteDynamicsWorld* world = m_impl->world.get();
		if (const int left = world->getNumCollisionObjects(); left > 0)
		{
			Log::Warning("PhysicsWorld: %d collision objects still registered at shutdown, detaching them", left);
			for (int i = world->getNumCollisionObjects() - 1; i >= 0; --i)
				world->removeCollisionObject(world->getCollisionObjectArray()[i]);
		}

		m_impl.reset();
	}

	bool PhysicsWorld::IsReady() const
	{
		return m_impl != nullptr;
	}

	void PhysicsWorld::Step(float fixedDeltaTime)
	{
		if (m_impl)
			m_impl->world->stepSimulation(fixedDeltaTime, 1, fixedDeltaTime);
	}

	void PhysicsWorld::SetGravity(const Vector3& gravity)
	{
		if (m_impl)
			m_impl->world->setGravity(bt::ToBt(gravity));
	}

	Vector3 PhysicsWorld::GetGravity() const
	{
		return m_impl ? bt::ToRaylib(m_impl->world->getGravity()) : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	std::optional<RaycastHit> PhysicsWorld::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
													uint32_t mask) const
	{
		if (!m_impl || maxDistance <= 0.0f || Vector3LengthSqr(direction) < 1e-12f)
			return std::nullopt;

		const btVector3 from	= bt::ToBt(origin);
		const btVector3 to		= bt::ToBt(Vector3Add(origin, Vector3Scale(Vector3Normalize(direction), maxDistance)));

		btCollisionWorld::ClosestRayResultCallback callback(from, to);
		// Bullet tests (object group & ray mask) && (ray group & object mask): an all-bits ray group means
		// only `mask` decides what can be hit
		callback.m_collisionFilterGroup	= btBroadphaseProxy::AllFilter;
		callback.m_collisionFilterMask	= static_cast<int>(mask);
		m_impl->world->rayTest(from, to, callback);

		if (!callback.hasHit() || !callback.m_collisionObject)
			return std::nullopt;

		Collider* collider = static_cast<Collider*>(callback.m_collisionObject->getUserPointer());
		if (!collider)
			return std::nullopt;

		RaycastHit hit;
		hit.collider	= collider;
		hit.actor		= collider->GetOwner();
		hit.point		= bt::ToRaylib(callback.m_hitPointWorld);
		hit.normal		= bt::ToRaylib(callback.m_hitNormalWorld.normalized());
		hit.distance	= callback.m_closestHitFraction * maxDistance;
		return hit;
	}

	btDiscreteDynamicsWorld* PhysicsWorld::GetNativeWorld() const
	{
		return m_impl ? m_impl->world.get() : nullptr;
	}

	int PhysicsWorld::GetObjectCount() const
	{
		return m_impl ? m_impl->world->getNumCollisionObjects() : 0;
	}
}
