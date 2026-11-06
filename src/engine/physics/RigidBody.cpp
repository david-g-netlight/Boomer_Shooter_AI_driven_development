#include "BulletConvert.h"
#include "RigidBody.h"
#include "ActorMotionState.h"
#include "Collider.h"
#include "PhysicsWorld.h"
#include <btBulletDynamicsCommon.h>
#include "core/Engine.h"
#include "core/Log.h"
#include "level/GameActor.h"
#include <algorithm>

namespace eng
{
	struct RigidBody::Impl
	{
		std::unique_ptr<ActorMotionState>	motionState;
		std::unique_ptr<btRigidBody>		body;
	};

	RigidBody::RigidBody()
		: m_impl(std::make_unique<Impl>())
	{
	}

	RigidBody::RigidBody(const RigidBodySettings& settings)
		: m_impl(std::make_unique<Impl>())
		, m_settings(settings)
	{
	}

	RigidBody::~RigidBody()
	{
		DestroyBody();
	}

	//////////////////////////////////////////
	/// Component
	//////////////////////////////////////////
	void RigidBody::OnEnable()
	{
		CreateBody();
	}

	void RigidBody::OnDisable()
	{
		DestroyBody();
	}

	void RigidBody::CreateBody()
	{
		GameActor* owner = GetOwner();
		if (!owner || m_impl->body)
			return;

		m_collider = owner->GetComponent<Collider>();
		if (!m_collider)
		{
			Log::Warning("RigidBody on '%s' has no Collider (add the Collider first)", owner->GetName().c_str());
			return;
		}

		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		btDiscreteDynamicsWorld* world = physics.GetNativeWorld();
		if (!world)
			return;

		m_collider->SyncScaleFromActor();
		btCollisionShape* shape = m_collider->GetNativeShape();

		const bool		isDynamic	= m_settings.type == BodyType::Dynamic;
		const btScalar	mass		= isDynamic ? std::max(m_settings.mass, 0.001f) : 0.0f;
		btVector3		inertia(0.0f, 0.0f, 0.0f);
		if (isDynamic)
			shape->calculateLocalInertia(mass, inertia);

		m_impl->motionState = std::make_unique<ActorMotionState>(*owner);
		btRigidBody::btRigidBodyConstructionInfo info(mass, m_impl->motionState.get(), shape, inertia);
		m_impl->body = std::make_unique<btRigidBody>(info);

		btRigidBody* body = m_impl->body.get();
		body->setUserPointer(m_collider);	// ray hits and trigger overlaps map back to the actor through this
		if (m_settings.type == BodyType::Kinematic)
		{
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			body->setActivationState(DISABLE_DEACTIVATION);
		}
		else if (m_settings.type == BodyType::Static)
		{
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
		}
		ApplySettings();
		UpdateCcd();

		world->addRigidBody(body, static_cast<int>(m_collider->GetLayer()), static_cast<int>(m_collider->GetMask()));
	}

	void RigidBody::DestroyBody()
	{
		if (!m_impl->body)
			return;

		// Always out of the world before deletion (the world may already be shut down)
		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		if (btDiscreteDynamicsWorld* world = physics.GetNativeWorld())
			world->removeRigidBody(m_impl->body.get());
		physics.OnColliderRemoved(m_collider);	// no trigger may keep a pointer to it

		m_impl->body.reset();
		m_impl->motionState.reset();
	}

	void RigidBody::ApplySettings()
	{
		btRigidBody* body = m_impl->body.get();
		if (!body)
			return;

		body->setFriction(m_settings.friction);
		body->setRestitution(m_settings.restitution);
		body->setDamping(m_settings.linearDamping, m_settings.angularDamping);
		body->setAngularFactor(m_settings.lockRotation ? 0.0f : 1.0f);
		if (m_settings.lockRotation)
			body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));	// the factor only scales impulses/torques
	}

	void RigidBody::UpdateCcd()
	{
		btRigidBody* body = m_impl->body.get();
		if (!body)
			return;

		// Smallest dimension of the (scaled) shape
		btVector3 lo, hi;
		body->getCollisionShape()->getAabb(btTransform::getIdentity(), lo, hi);
		const btVector3	size	= hi - lo;
		const float		minSize	= std::min({ size.x(), size.y(), size.z() });

		const bool active = m_settings.type == BodyType::Dynamic
			&& (m_settings.ccd == CcdMode::On || (m_settings.ccd == CcdMode::Auto && minSize < CCD_AUTO_SIZE));

		// Engages when a step moves more than half the smallest size; the swept sphere fits inside the body
		body->setCcdMotionThreshold(active ? 0.5f * minSize : 0.0f);
		body->setCcdSweptSphereRadius(active ? 0.4f * minSize : 0.0f);
	}

	void RigidBody::SetCcdMode(CcdMode mode)
	{
		m_settings.ccd = mode;
		UpdateCcd();
	}

	bool RigidBody::IsCcdActive() const
	{
		return m_impl->body && m_impl->body->getCcdMotionThreshold() > 0.0f;
	}

	void RigidBody::UpdateMassProperties()
	{
		btRigidBody* body = m_impl->body.get();
		if (!body || m_settings.type != BodyType::Dynamic)
			return;

		btVector3 inertia(0.0f, 0.0f, 0.0f);
		const btScalar mass = std::max(m_settings.mass, 0.001f);
		body->getCollisionShape()->calculateLocalInertia(mass, inertia);
		body->setMassProps(mass, inertia);
		body->updateInertiaTensor();
		body->activate(true);
	}

	//////////////////////////////////////////
	/// Settings
	//////////////////////////////////////////
	void RigidBody::SetType(BodyType type)
	{
		if (type == m_settings.type)
			return;

		m_settings.type = type;
		if (m_impl->body)
		{
			// Mass, flags and broadphase handling all depend on the type: rebuild
			DestroyBody();
			CreateBody();
		}
	}

	BodyType RigidBody::GetType() const							{ return m_settings.type; }
	float RigidBody::GetMass() const							{ return m_settings.mass; }
	const RigidBodySettings& RigidBody::GetSettings() const		{ return m_settings; }

	void RigidBody::SetMass(float mass)							{ m_settings.mass = mass; UpdateMassProperties(); }
	void RigidBody::SetFriction(float friction)					{ m_settings.friction = friction; ApplySettings(); }
	void RigidBody::SetRestitution(float restitution)			{ m_settings.restitution = restitution; ApplySettings(); }
	void RigidBody::SetLockRotation(bool locked)				{ m_settings.lockRotation = locked; ApplySettings(); }

	void RigidBody::SetDamping(float linear, float angular)
	{
		m_settings.linearDamping	= linear;
		m_settings.angularDamping	= angular;
		ApplySettings();
	}

	//////////////////////////////////////////
	/// Motion
	//////////////////////////////////////////
	void RigidBody::AddImpulse(const Vector3& impulse, const Vector3& relativePoint)
	{
		if (btRigidBody* body = m_impl->body.get())
		{
			body->activate(true);
			body->applyImpulse(bt::ToBt(impulse), bt::ToBt(relativePoint));
		}
	}

	void RigidBody::AddForce(const Vector3& force)
	{
		if (btRigidBody* body = m_impl->body.get())
		{
			body->activate(true);
			body->applyCentralForce(bt::ToBt(force));
		}
	}

	void RigidBody::SetLinearVelocity(const Vector3& velocity)
	{
		if (btRigidBody* body = m_impl->body.get())
		{
			body->activate(true);
			body->setLinearVelocity(bt::ToBt(velocity));
		}
	}

	Vector3 RigidBody::GetLinearVelocity() const
	{
		return m_impl->body ? bt::ToRaylib(m_impl->body->getLinearVelocity()) : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	void RigidBody::SetAngularVelocity(const Vector3& velocity)
	{
		// A direct velocity bypasses Bullet's angular factor: a locked body must ignore it
		if (m_settings.lockRotation)
			return;

		if (btRigidBody* body = m_impl->body.get())
		{
			body->activate(true);
			body->setAngularVelocity(bt::ToBt(velocity));
		}
	}

	Vector3 RigidBody::GetAngularVelocity() const
	{
		return m_impl->body ? bt::ToRaylib(m_impl->body->getAngularVelocity()) : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	void RigidBody::Teleport(const ::Transform& transform)
	{
		GameActor* owner = GetOwner();
		if (!owner)
			return;

		owner->GetTransform() = transform;

		btRigidBody* body = m_impl->body.get();
		if (!body)
			return;

		btTransform pose;
		m_impl->motionState->getWorldTransform(pose);	// world pose of the actor we just moved
		body->setWorldTransform(pose);
		body->setInterpolationWorldTransform(pose);
		body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
		body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
		body->clearForces();
		body->activate(true);

		if (btDiscreteDynamicsWorld* world = Engine::GetInstance().GetPhysics().GetNativeWorld())
			world->updateSingleAabb(body);
	}

	void RigidBody::OnScaleChanged()
	{
		if (!m_collider)
			return;

		m_collider->SyncScaleFromActor();
		UpdateMassProperties();
		UpdateCcd();	// a shrunk prop may now need CCD
		if (btRigidBody* body = m_impl->body.get())
		{
			body->activate(true);
			if (btDiscreteDynamicsWorld* world = Engine::GetInstance().GetPhysics().GetNativeWorld())
				world->updateSingleAabb(body);
		}
	}

	bool RigidBody::IsSleeping() const
	{
		return m_impl->body && m_impl->body->getActivationState() == ISLAND_SLEEPING;
	}

	void RigidBody::WakeUp()
	{
		if (m_impl->body)
			m_impl->body->activate(true);
	}

	bool RigidBody::HasBody() const
	{
		return m_impl->body != nullptr;
	}

	btRigidBody* RigidBody::GetNativeBody() const
	{
		return m_impl->body.get();
	}
}
