#pragma once
#include <cstdint>
#include <memory>
#include <raylib.h>
#include "level/Component.h"

class btRigidBody;	// Bullet type, only used by the engine's physics code

namespace eng
{
	class Collider;

	enum class BodyType : uint8_t
	{
		Static,		// never moves (level geometry); move it only with Teleport()
		Dynamic,	// simulated; writes its pose back to the actor
		Kinematic,	// moved by code through the actor's transform; pushes dynamic bodies
	};

	/// <summary>
	/// Continuous collision detection: stops small fast bodies from passing through thin geometry in one step.
	/// Auto = on for dynamic bodies whose smallest dimension is below CCD_AUTO_SIZE.
	/// </summary>
	enum class CcdMode : uint8_t
	{
		Auto,
		On,
		Off,
	};

	constexpr float CCD_AUTO_SIZE = 0.3f;	// m

	struct RigidBodySettings
	{
		BodyType	type			= BodyType::Dynamic;
		float		mass			= 1.0f;		// kg, dynamic only
		float		friction		= 0.6f;
		float		restitution		= 0.0f;
		float		linearDamping	= 0.05f;
		float		angularDamping	= 0.1f;
		bool		lockRotation	= false;	// angular factor 0: never tips over
		CcdMode		ccd				= CcdMode::Auto;
	};

	/// <summary>
	/// A Bullet rigid body for the Collider on the same actor (add the Collider first).
	/// The body is created and added to the world in OnEnable, removed and deleted in OnDisable (which also
	/// runs when the actor is destroyed), so a destroyed actor never leaves a body behind.
	/// Settings changed while enabled apply immediately; a type change rebuilds the body.
	/// </summary>
	class RigidBody : public Component
	{
	public:
		RigidBody();
		explicit RigidBody(const RigidBodySettings& settings);
		~RigidBody() override;

		//////////////////////////////////////////
		/// Settings
		//////////////////////////////////////////
		void				SetType(BodyType type);
		BodyType			GetType() const;
		void				SetMass(float mass);
		float				GetMass() const;
		void				SetFriction(float friction);
		void				SetRestitution(float restitution);
		void				SetDamping(float linear, float angular);
		void				SetLockRotation(bool locked);
		void				SetCcdMode(CcdMode mode);
		bool				IsCcdActive() const;
		const RigidBodySettings& GetSettings() const;

		//////////////////////////////////////////
		/// Motion
		//////////////////////////////////////////
		void				AddImpulse(const Vector3& impulse, const Vector3& relativePoint = { 0.0f, 0.0f, 0.0f });
		void				AddForce(const Vector3& force);
		void				SetLinearVelocity(const Vector3& velocity);
		Vector3				GetLinearVelocity() const;
		void				SetAngularVelocity(const Vector3& velocity);	// ignored while rotation is locked
		Vector3				GetAngularVelocity() const;

		/// <summary>
		/// Moves actor and body together and clears velocities (spawning, editor moves, respawn).
		/// keepVelocity: leave the velocities alone (a prop that is being scaled and shifted while it moves).
		/// </summary>
		void				Teleport(const ::Transform& transform, bool keepVelocity = false);

		/// <summary>
		/// Call after the actor's scale changed: rescales the collider, recomputes mass properties and the AABB.
		/// </summary>
		void				OnScaleChanged();

		bool				IsSleeping() const;
		void				WakeUp();
		bool				HasBody() const;

		/// <summary>
		/// ENGINE-INTERNAL.
		/// </summary>
		btRigidBody*		GetNativeBody() const;

		//////////////////////////////////////////
		/// Component
		//////////////////////////////////////////
		void				OnEnable() override;
		void				OnDisable() override;
		void			Serialize(PropertyBag& out) const override;
		void			Deserialize(const PropertyBag& in) override;
		const char*			TypeName() const override { return "RigidBody"; }

	private:
		void				CreateBody();
		void				DestroyBody();
		void				ApplySettings();
		void				UpdateMassProperties();
		void				UpdateCcd();

		struct Impl;
		std::unique_ptr<Impl>	m_impl;
		RigidBodySettings		m_settings;
		Collider*				m_collider		= nullptr;
	};
}
