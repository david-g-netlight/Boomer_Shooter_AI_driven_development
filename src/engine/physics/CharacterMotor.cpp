#include "BulletConvert.h"
#include "CharacterMotor.h"
#include "ActorMotionState.h"
#include "Collider.h"
#include "PhysicsWorld.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "level/GameActor.h"
#include <btBulletDynamicsCommon.h>
#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace eng
{
	struct CharacterMotor::Impl
	{
		std::unique_ptr<ActorMotionState>	motionState;
		std::unique_ptr<btRigidBody>		body;
		std::unique_ptr<btSphereShape>		probe;		// ground sweep sphere
	};

	CharacterMotor::CharacterMotor()
		: m_impl(std::make_unique<Impl>())
	{
	}

	CharacterMotor::CharacterMotor(const CharacterMotorSettings& settings)
		: m_impl(std::make_unique<Impl>())
		, m_settings(settings)
	{
	}

	CharacterMotor::~CharacterMotor()
	{
		DestroyBody();
	}

	//////////////////////////////////////////
	/// Component
	//////////////////////////////////////////
	void CharacterMotor::OnEnable()		{ CreateBody(); }
	void CharacterMotor::OnDisable()	{ DestroyBody(); }

	void CharacterMotor::CreateBody()
	{
		GameActor* owner = GetOwner();
		if (!owner || m_impl->body)
			return;

		m_collider = owner->GetComponent<Collider>();
		if (!m_collider || m_collider->GetShapeType() != ColliderShape::Capsule)
		{
			Log::Warning("CharacterMotor on '%s' needs a capsule Collider (add it first)", owner->GetName().c_str());
			m_collider = nullptr;
			return;
		}

		btDiscreteDynamicsWorld* world = Engine::GetInstance().GetPhysics().GetNativeWorld();
		if (!world)
			return;

		btCollisionShape* shape = m_collider->GetNativeShape();
		m_impl->motionState	= std::make_unique<ActorMotionState>(*owner, false);	// never overwrite the yaw
		m_impl->probe		= std::make_unique<btSphereShape>(0.95f * m_collider->GetRadius());

		// Inertia is irrelevant: rotation is locked
		btRigidBody::btRigidBodyConstructionInfo info(m_settings.mass, m_impl->motionState.get(), shape, btVector3(0, 0, 0));
		info.m_friction		= 0.0f;	// walking is velocity-driven; friction would make it stick to walls
		info.m_restitution	= 0.0f;
		m_impl->body = std::make_unique<btRigidBody>(info);

		btRigidBody* body = m_impl->body.get();
		body->setUserPointer(m_collider);
		body->setAngularFactor(0.0f);
		body->setActivationState(DISABLE_DEACTIVATION);
		body->setCcdMotionThreshold(0.5f * m_collider->GetRadius());
		body->setCcdSweptSphereRadius(0.5f * m_collider->GetRadius());
		// Own gravity: addRigidBody would otherwise overwrite it with the world's
		body->setFlags(body->getFlags() | BT_DISABLE_WORLD_GRAVITY);
		world->addRigidBody(body, static_cast<int>(m_collider->GetLayer()), static_cast<int>(m_collider->GetMask()));
		body->setGravity(btVector3(0.0f, -m_settings.gravity, 0.0f));
	}

	void CharacterMotor::DestroyBody()
	{
		if (!m_impl->body)
			return;

		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		if (btDiscreteDynamicsWorld* world = physics.GetNativeWorld())
			world->removeRigidBody(m_impl->body.get());
		physics.OnColliderRemoved(m_collider);

		m_impl->body.reset();
		m_impl->motionState.reset();
		m_impl->probe.reset();
		m_isGrounded = false;
	}

	//////////////////////////////////////////
	/// Control
	//////////////////////////////////////////
	void CharacterMotor::SetWalkVelocity(const Vector3& velocity)
	{
		m_walkVelocity = { velocity.x, 0.0f, velocity.z };
	}

	bool CharacterMotor::Jump(float speed)
	{
		if (!m_isGrounded || speed <= 0.0f)
			return false;
		m_jumpRequest = speed;
		return true;
	}

	void CharacterMotor::SetGravity(float gravity)
	{
		m_settings.gravity = gravity;
		if (m_impl->body)
			m_impl->body->setGravity(btVector3(0.0f, -gravity, 0.0f));
	}

	void CharacterMotor::Warp(const Vector3& position)
	{
		GameActor* owner = GetOwner();
		if (!owner)
			return;

		owner->SetPosition(position);
		if (btRigidBody* body = m_impl->body.get())
		{
			btTransform pose;
			m_impl->motionState->getWorldTransform(pose);
			body->setWorldTransform(pose);
			body->setInterpolationWorldTransform(pose);
			body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
			if (btDiscreteDynamicsWorld* world = Engine::GetInstance().GetPhysics().GetNativeWorld())
				world->updateSingleAabb(body);
		}
		m_isGrounded	= false;
		m_jumpRequest	= 0.0f;
		m_walkVelocity	= { 0.0f, 0.0f, 0.0f };	// a teleport starts from rest
	}

	bool CharacterMotor::IsGrounded() const							{ return m_isGrounded; }
	bool CharacterMotor::IsOnSteepSlope() const						{ return m_isOnSteepSlope; }
	const Vector3& CharacterMotor::GetGroundNormal() const			{ return m_groundNormal; }
	const CharacterMotorSettings& CharacterMotor::GetSettings() const	{ return m_settings; }

	float CharacterMotor::GetGroundSlopeDegrees() const
	{
		return std::acos(std::clamp(m_groundNormal.y, -1.0f, 1.0f)) * RAD2DEG;
	}

	Vector3 CharacterMotor::GetVelocity() const
	{
		return m_impl->body ? bt::ToRaylib(m_impl->body->getLinearVelocity()) : Vector3{ 0.0f, 0.0f, 0.0f };
	}

	//////////////////////////////////////////
	/// Simulation
	//////////////////////////////////////////
	void CharacterMotor::ProbeGround()
	{
		m_isGrounded		= false;
		m_isOnSteepSlope	= false;
		m_groundNormal		= { 0.0f, 1.0f, 0.0f };
		m_groundVelocity	= { 0.0f, 0.0f, 0.0f };

		btDiscreteDynamicsWorld* world = Engine::GetInstance().GetPhysics().GetNativeWorld();
		if (!world)
			return;

		// Sweep a slightly smaller sphere from the capsule's bottom sphere straight down
		const btVector3 center		= m_impl->body->getWorldTransform().getOrigin();
		const float		radius		= m_collider->GetRadius();
		const float		sweepRadius	= m_impl->probe->getRadius();
		const btVector3 from		= center - btVector3(0.0f, 0.5f * m_collider->GetHeight(), 0.0f);
		const btVector3 to			= from - btVector3(0.0f, (radius - sweepRadius) + m_settings.groundProbe, 0.0f);

		btCollisionWorld::ClosestConvexResultCallback hit(from, to);
		hit.m_collisionFilterGroup	= btBroadphaseProxy::AllFilter;
		hit.m_collisionFilterMask	= static_cast<int>(Layer::Ground | Layer::Prop);	// never the player or triggers
		world->convexSweepTest(m_impl->probe.get(),
			btTransform(btQuaternion::getIdentity(), from), btTransform(btQuaternion::getIdentity(), to), hit);

		if (!hit.hasHit())
			return;

		const Vector3 normal = bt::ToRaylib(hit.m_hitNormalWorld.normalized());
		if (normal.y >= std::cos(m_settings.maxSlopeDegrees * DEG2RAD))
		{
			m_isGrounded	= true;
			m_groundNormal	= normal;
			if (const btRigidBody* ground = btRigidBody::upcast(hit.m_hitCollisionObject))
				m_groundVelocity = bt::ToRaylib(ground->getLinearVelocity());
		}
		else if (normal.y > 0.0f)
		{
			m_isOnSteepSlope	= true;
			m_groundNormal		= normal;
		}
	}

	void CharacterMotor::FixedUpdate(float fixedDeltaTime)
	{
		btRigidBody* body = m_impl->body.get();
		if (!body)
			return;

		m_jumpGraceLeft = std::max(0.0f, m_jumpGraceLeft - fixedDeltaTime);
		ProbeGround();
		if (m_jumpGraceLeft > 0.0f)
			m_isGrounded = false;	// just jumped: don't snap back to the ground

		Vector3 desired	= m_walkVelocity;
		Vector3 velocity = bt::ToRaylib(body->getLinearVelocity());

		// Too steep to stand on: drop the part of the input pushing into the slope, otherwise the frictionless
		// contact turns horizontal push into upward speed and the capsule climbs it
		if (m_isOnSteepSlope)
		{
			Vector3 into = Vector3Normalize({ -m_groundNormal.x, 0.0f, -m_groundNormal.z });
			const float push = Vector3DotProduct(desired, into);
			if (push > 0.0f)
				desired = Vector3Subtract(desired, Vector3Scale(into, push));
		}

		if (m_isGrounded)
		{
			// Follow the ground plane at the requested speed (no bouncing on slope changes)
			const float speed = Vector3Length(desired);
			Vector3 along = Vector3Subtract(desired, Vector3Scale(m_groundNormal, Vector3DotProduct(desired, m_groundNormal)));
			if (Vector3LengthSqr(along) > 1e-8f)
				along = Vector3Scale(Vector3Normalize(along), speed);

			velocity = Vector3Add(along, { m_groundVelocity.x, 0.0f, m_groundVelocity.z });
			velocity.y += m_settings.groundGlue;

			if (m_jumpRequest > 0.0f)
			{
				velocity.y		= m_jumpRequest;
				m_jumpGraceLeft	= m_settings.jumpGraceSeconds;
				m_isGrounded	= false;
			}
		}
		else
		{
			// Airborne: steer toward the input, keep falling
			const float blend = std::min(1.0f, m_settings.airControl * fixedDeltaTime / 0.1f);
			velocity.x += (desired.x - velocity.x) * blend;
			velocity.z += (desired.z - velocity.z) * blend;
		}
		m_jumpRequest = 0.0f;

		velocity.y = std::clamp(velocity.y, -m_settings.maxVerticalSpeed, m_settings.maxVerticalSpeed);
		body->setLinearVelocity(bt::ToBt(velocity));

		// Grounded: gravity pushes into the ground (no component along the slope), so standing still on a
		// frictionless slope doesn't slide. Airborne: straight down.
		const Vector3 gravity = m_isGrounded ? Vector3Scale(m_groundNormal, -m_settings.gravity)
											 : Vector3{ 0.0f, -m_settings.gravity, 0.0f };
		body->setGravity(bt::ToBt(gravity));
		body->activate(true);
	}
}
