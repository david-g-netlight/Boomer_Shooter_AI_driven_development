#pragma once
#include <cstdint>
#include <memory>
#include <raylib.h>
#include "level/Component.h"
#include "physics/PhysicsLayers.h"

class btCollisionShape;	// Bullet type, only used by the engine's physics code

namespace eng
{
	enum class ColliderShape : uint8_t
	{
		Box,
		Sphere,
		Capsule,		// Y axis: two spheres of `radius`, `height` apart (total height = height + 2 * radius)
		ConvexHull,		// from a mesh's vertices, for dynamic props with odd shapes
		StaticMesh,		// exact triangles, static level geometry only (don't scale at runtime)
	};

	/// <summary>
	/// The collision shape of an actor, used by RigidBody / TriggerVolume on the same actor.
	/// Each Collider owns its own Bullet shape, so scaling one prop never scales another.
	///
	/// Center offset: the shape is wrapped in a compound so the offset scales too. SetBottomPivot(true)
	/// sets the center to half the shape's height: the bottom face stays on the actor's origin, and a
	/// stretched prop grows upward instead of into the floor.
	///
	/// Configure the shape before the body is enabled; changing it later only applies after the body is
	/// re-enabled.
	/// </summary>
	class Collider : public Component
	{
	public:
		Collider();
		~Collider() override;

		//////////////////////////////////////////
		/// Shape
		//////////////////////////////////////////
		void			SetBox(const Vector3& halfExtents);
		void			SetSphere(float radius);
		void			SetCapsule(float radius, float height);
		void			SetConvexHull(const ::Mesh& mesh);
		void			SetStaticMesh(const ::Mesh& mesh);

		void			SetCenter(const Vector3& center);		// local offset, scaled with the shape
		void			SetBottomPivot(bool bottomPivot);		// center = (0, half height, 0)
		const Vector3&	GetCenter() const;

		ColliderShape	GetShapeType() const;
		const Vector3&	GetHalfExtents() const;				// box
		float			GetRadius() const;					// sphere / capsule
		float			GetHeight() const;					// capsule

		//////////////////////////////////////////
		/// Scale (follows the actor's world scale)
		//////////////////////////////////////////
		void			SetScale(const Vector3& scale);
		const Vector3&	GetScale() const;
		void			SyncScaleFromActor();

		//////////////////////////////////////////
		/// Filtering
		//////////////////////////////////////////
		void			SetLayer(uint32_t layer);	// also resets the mask to DefaultCollisionMask(layer)
		uint32_t		GetLayer() const;
		void			SetMask(uint32_t mask);
		uint32_t		GetMask() const;

		/// <summary>
		/// World-space AABB of the shape at the actor's current transform (debug, editor).
		/// </summary>
		BoundingBox		GetWorldBounds() const;

		/// <summary>
		/// ENGINE-INTERNAL. Built on first use.
		/// </summary>
		btCollisionShape*	GetNativeShape();

		const char*		TypeName() const override { return "Collider"; }

	private:
		void			Rebuild();

		struct Impl;
		std::unique_ptr<Impl>	m_impl;

		ColliderShape	m_shape			= ColliderShape::Box;
		Vector3			m_halfExtents	= { 0.5f, 0.5f, 0.5f };
		float			m_radius		= 0.5f;
		float			m_height		= 1.0f;
		const ::Mesh*	m_mesh			= nullptr;
		Vector3			m_center		= { 0.0f, 0.0f, 0.0f };
		bool			m_isBottomPivot	= false;
		Vector3			m_scale			= { 1.0f, 1.0f, 1.0f };
		uint32_t		m_layer			= Layer::Prop;
		uint32_t		m_mask			= DefaultCollisionMask(Layer::Prop);
		bool			m_isDirty		= true;
	};
}
