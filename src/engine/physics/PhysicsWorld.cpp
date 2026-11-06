#include "PhysicsWorldImpl.h"
#include "core/Log.h"
#include "physics/Collider.h"
#include "level/GameActor.h"
#include "physics/ActorMotionState.h"
#include <algorithm>
#include <vector>
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
		// Keeps each ghost object's overlapping-pair list up to date (TriggerVolume)
		m_impl->ghostPairCallback = std::make_unique<btGhostPairCallback>();
		m_impl->broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(m_impl->ghostPairCallback.get());
		m_impl->solver			= std::make_unique<btSequentialImpulseConstraintSolver>();
		m_impl->world			= std::make_unique<btDiscreteDynamicsWorld>(
			m_impl->dispatcher.get(), m_impl->broadphase.get(), m_impl->solver.get(), m_impl->configuration.get());

		m_impl->world->setGravity(bt::ToBt(gravity));
		m_impl->debugDrawer = std::make_unique<PhysicsDebugDrawer>();
		m_impl->world->setDebugDrawer(m_impl->debugDrawer.get());
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
		if (!m_impl)
			return;

		SyncTriggers();
		m_impl->world->stepSimulation(fixedDeltaTime, 1, fixedDeltaTime);
		DispatchTriggers();
	}

	//////////////////////////////////////////
	/// Triggers
	//////////////////////////////////////////
	void PhysicsWorld::RegisterTrigger(TriggerVolume& trigger)
	{
		if (!m_impl)
			return;

		const Collider* collider = trigger.m_collider;
		m_impl->world->addCollisionObject(trigger.m_impl->ghost.get(),
			static_cast<int>(collider->GetLayer()), static_cast<int>(collider->GetMask()));
		m_impl->triggers.push_back(&trigger);
	}

	void PhysicsWorld::UnregisterTrigger(TriggerVolume& trigger)
	{
		if (!m_impl)
			return;

		auto& triggers = m_impl->triggers;
		triggers.erase(std::remove(triggers.begin(), triggers.end(), &trigger), triggers.end());
		m_impl->world->removeCollisionObject(trigger.m_impl->ghost.get());
		OnColliderRemoved(trigger.m_collider);	// other triggers may contain this trigger's collider
	}

	void PhysicsWorld::SyncTriggers()
	{
		for (TriggerVolume* trigger : m_impl->triggers)
		{
			GameActor* owner = trigger->GetOwner();
			if (!owner)
				continue;
			btTransform pose;
			ActorMotionState(*owner).getWorldTransform(pose);
			trigger->m_impl->ghost->setWorldTransform(pose);
		}
	}

	namespace
	{
		// Exact test for one candidate pair (the ghost's pair list is only broadphase / AABB overlap)
		struct TouchCallback : btCollisionWorld::ContactResultCallback
		{
			bool touching = false;
			btScalar addSingleResult(btManifoldPoint& point, const btCollisionObjectWrapper*, int, int,
									 const btCollisionObjectWrapper*, int, int) override
			{
				if (point.getDistance() <= 0.0f)
					touching = true;
				return 0.0f;
			}
		};
	}

	void PhysicsWorld::DispatchTriggers()
	{
		// Copy: callbacks may enable / disable triggers
		const std::vector<TriggerVolume*> triggers = m_impl->triggers;

		for (TriggerVolume* trigger : triggers)
		{
			if (std::find(m_impl->triggers.begin(), m_impl->triggers.end(), trigger) == m_impl->triggers.end())
				continue;	// removed by an earlier callback this tick

			btPairCachingGhostObject* ghost = trigger->m_impl->ghost.get();
			std::unordered_set<Collider*> now;
			for (int i = 0; i < ghost->getNumOverlappingObjects(); ++i)
			{
				btCollisionObject* other = ghost->getOverlappingObject(i);
				Collider* collider = static_cast<Collider*>(other->getUserPointer());
				if (!collider || collider == trigger->m_collider || !collider->GetOwner())
					continue;

				TouchCallback touch;
				m_impl->world->contactPairTest(ghost, other, touch);
				if (touch.touching)
					now.insert(collider);
			}

			GameActor* triggerActor = trigger->GetOwner();
			std::vector<Collider*> entered, exited;
			for (Collider* collider : now)
				if (!trigger->m_inside.count(collider))
					entered.push_back(collider);
			for (Collider* collider : trigger->m_inside)
				if (!now.count(collider))
					exited.push_back(collider);
			trigger->m_inside = std::move(now);

			for (Collider* collider : entered)
			{
				GameActor* other = collider->GetOwner();
				triggerActor->NotifyTriggerEnter(*other);
				other->NotifyTriggerEnter(*triggerActor);
			}
			for (Collider* collider : exited)
			{
				GameActor* other = collider->GetOwner();
				triggerActor->NotifyTriggerExit(*other);
				other->NotifyTriggerExit(*triggerActor);
			}
		}
	}

	void PhysicsWorld::OnColliderRemoved(const Collider* collider)
	{
		if (!m_impl || !collider)
			return;
		for (TriggerVolume* trigger : m_impl->triggers)
			trigger->m_inside.erase(const_cast<Collider*>(collider));
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

	void PhysicsWorld::SetDebugDrawEnabled(bool enabled)
	{
		if (m_impl)
			m_impl->isDebugDrawEnabled = enabled;
	}

	bool PhysicsWorld::IsDebugDrawEnabled() const
	{
		return m_impl && m_impl->isDebugDrawEnabled;
	}

	void PhysicsWorld::DebugDraw() const
	{
		if (m_impl && m_impl->isDebugDrawEnabled)
			m_impl->world->debugDrawWorld();
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
