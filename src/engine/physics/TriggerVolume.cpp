#include "BulletConvert.h"
#include "TriggerVolume.h"
#include "ActorMotionState.h"
#include "Collider.h"
#include "PhysicsWorldImpl.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "level/GameActor.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

namespace eng
{
	TriggerVolume::TriggerVolume()
		: m_impl(std::make_unique<Impl>())
	{
	}

	TriggerVolume::~TriggerVolume()
	{
		OnDisable();
	}

	size_t TriggerVolume::GetOverlapCount() const
	{
		return m_inside.size();
	}

	bool TriggerVolume::Contains(const GameActor& actor) const
	{
		for (const Collider* collider : m_inside)
			if (collider->GetOwner() == &actor)
				return true;
		return false;
	}

	void TriggerVolume::OnEnable()
	{
		GameActor* owner = GetOwner();
		if (!owner || m_impl->ghost)
			return;

		m_collider = owner->GetComponent<Collider>();
		if (!m_collider)
		{
			Log::Warning("TriggerVolume on '%s' has no Collider (add the Collider first)", owner->GetName().c_str());
			return;
		}

		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		if (!physics.GetNativeWorld())
			return;

		m_collider->SyncScaleFromActor();
		m_impl->ghost = std::make_unique<btPairCachingGhostObject>();
		btPairCachingGhostObject* ghost = m_impl->ghost.get();
		ghost->setCollisionShape(m_collider->GetNativeShape());
		ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
		ghost->setUserPointer(m_collider);

		btTransform pose;
		ActorMotionState(*owner).getWorldTransform(pose);
		ghost->setWorldTransform(pose);

		physics.RegisterTrigger(*this);
	}

	void TriggerVolume::OnDisable()
	{
		if (!m_impl->ghost)
			return;

		Engine::GetInstance().GetPhysics().UnregisterTrigger(*this);
		m_impl->ghost.reset();
		m_inside.clear();
	}

}
