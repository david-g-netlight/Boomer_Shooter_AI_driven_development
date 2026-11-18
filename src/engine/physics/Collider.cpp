#include "BulletConvert.h"
#include "Collider.h"
#include "level/GameActor.h"
#include "core/Log.h"
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <raymath.h>
#include "core/MathUtils.h"
#include "editor/InspectorLayout.h"
#include "level/PropertyBag.h"
#include "physics/RigidBody.h"
#include "physics/TriggerVolume.h"

namespace eng
{
	namespace
	{
		constexpr float CONVEX_HULL_MARGIN = 0.01f;
	}

	struct Collider::Impl
	{
		std::unique_ptr<btCollisionShape>	shape;		// the child shape
		std::unique_ptr<btCompoundShape>	compound;	// wraps `shape` when there is a center offset
		std::unique_ptr<btTriangleMesh>		triangles;	// StaticMesh data, must outlive its shape

		btCollisionShape* Root() const { return compound ? compound.get() : shape.get(); }
	};

	Collider::Collider()
		: m_impl(std::make_unique<Impl>())
	{
	}

	Collider::~Collider() = default;

	//////////////////////////////////////////
	/// Shape
	//////////////////////////////////////////
	void Collider::SetBox(const Vector3& halfExtents)
	{
		m_shape = ColliderShape::Box; m_halfExtents = halfExtents; m_isDirty = true;
	}

	void Collider::SetSphere(float radius)
	{
		m_shape = ColliderShape::Sphere; m_radius = radius; m_isDirty = true;
	}

	void Collider::SetCapsule(float radius, float height)
	{
		m_shape = ColliderShape::Capsule; m_radius = radius; m_height = height; m_isDirty = true;
	}

	void Collider::SetConvexHull(const ::Mesh& mesh)
	{
		m_shape = ColliderShape::ConvexHull; m_mesh = &mesh; m_isDirty = true;
	}

	void Collider::SetStaticMesh(const ::Mesh& mesh)
	{
		m_shape = ColliderShape::StaticMesh; m_mesh = &mesh; m_isDirty = true;
	}

	void Collider::SetCenter(const Vector3& center)
	{
		m_center = center; m_isBottomPivot = false; m_isDirty = true;
	}

	void Collider::SetBottomPivot(bool bottomPivot)
	{
		m_isBottomPivot = bottomPivot; m_isDirty = true;
	}

	bool Collider::IsBottomPivot() const				{ return m_isBottomPivot; }
	const Vector3& Collider::GetCenter() const			{ return m_center; }
	ColliderShape Collider::GetShapeType() const		{ return m_shape; }
	const Vector3& Collider::GetHalfExtents() const		{ return m_halfExtents; }
	float Collider::GetRadius() const					{ return m_radius; }
	float Collider::GetHeight() const					{ return m_height; }

	//////////////////////////////////////////
	/// Scale
	//////////////////////////////////////////
	void Collider::SetScale(const Vector3& scale)
	{
		m_scale = scale;
		if (btCollisionShape* root = m_impl->Root())
			root->setLocalScaling(bt::ToBt(scale));
	}

	const Vector3& Collider::GetScale() const
	{
		return m_scale;
	}

	void Collider::SyncScaleFromActor()
	{
		const GameActor* owner = GetOwner();
		if (!owner)
			return;

		// World scale = length of the world matrix's basis columns (lossy with skewed parents)
		const Matrix m = owner->GetWorldMatrix();
		SetScale({ Vector3Length({ m.m0, m.m1, m.m2 }),
				   Vector3Length({ m.m4, m.m5, m.m6 }),
				   Vector3Length({ m.m8, m.m9, m.m10 }) });
	}

	//////////////////////////////////////////
	/// Filtering
	//////////////////////////////////////////
	void Collider::SetLayer(uint32_t layer)	{ m_layer = layer; m_mask = DefaultCollisionMask(layer); }
	uint32_t Collider::GetLayer() const		{ return m_layer; }
	void Collider::SetMask(uint32_t mask)	{ m_mask = mask; }
	uint32_t Collider::GetMask() const		{ return m_mask; }

	BoundingBox Collider::GetWorldBounds() const
	{
		btCollisionShape* root = const_cast<Collider*>(this)->GetNativeShape();
		const GameActor* owner = GetOwner();
		if (!root || !owner)
			return { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

		::Transform pose = owner->GetTransform();
		const Matrix world = owner->GetWorldMatrix();
		Vector3 scale;
		DecomposeTransform(world, &pose.translation, &pose.rotation, &scale);

		btVector3 lo, hi;
		root->getAabb(bt::ToBt(pose), lo, hi);
		return { bt::ToRaylib(lo), bt::ToRaylib(hi) };
	}

	//////////////////////////////////////////
	/// Native shape
	//////////////////////////////////////////
	btCollisionShape* Collider::GetNativeShape()
	{
		if (m_isDirty)
			Rebuild();
		return m_impl->Root();
	}

	void Collider::Rebuild()
	{
		m_isDirty = false;
		m_impl->compound.reset();
		m_impl->shape.reset();
		m_impl->triangles.reset();

		float halfHeight = 0.0f;
		switch (m_shape)
		{
			case ColliderShape::Box:
				m_impl->shape = std::make_unique<btBoxShape>(bt::ToBt(m_halfExtents));
				halfHeight = m_halfExtents.y;
				break;

			case ColliderShape::Sphere:
				m_impl->shape = std::make_unique<btSphereShape>(m_radius);
				halfHeight = m_radius;
				break;

			case ColliderShape::Capsule:
				m_impl->shape = std::make_unique<btCapsuleShape>(m_radius, m_height);
				halfHeight = m_radius + 0.5f * m_height;
				break;

			case ColliderShape::ConvexHull:
			{
				if (!m_mesh || !m_mesh->vertices) { Log::Warning("Collider: convex hull without mesh data"); break; }
				auto hull = std::make_unique<btConvexHullShape>(m_mesh->vertices, m_mesh->vertexCount, 3 * sizeof(float));
				hull->optimizeConvexHull();
				hull->initializePolyhedralFeatures();
				// Hull margins sit OUTSIDE the points (box margins are inside): the default 4 cm would make
				// small props collide visibly "fat"
				hull->setMargin(CONVEX_HULL_MARGIN);
				const BoundingBox bounds = GetMeshBoundingBox(*m_mesh);
				halfHeight = 0.5f * (bounds.max.y - bounds.min.y);
				m_impl->shape = std::move(hull);
				break;
			}

			case ColliderShape::StaticMesh:
			{
				if (!m_mesh || !m_mesh->vertices) { Log::Warning("Collider: static mesh without mesh data"); break; }
				m_impl->triangles = std::make_unique<btTriangleMesh>();
				const float* v = m_mesh->vertices;
				auto vertex = [v](int i) { return btVector3(v[3 * i], v[3 * i + 1], v[3 * i + 2]); };
				const int triangleCount = m_mesh->indices ? m_mesh->triangleCount : m_mesh->vertexCount / 3;
				for (int t = 0; t < triangleCount; ++t)
				{
					const int a = m_mesh->indices ? m_mesh->indices[3 * t]     : 3 * t;
					const int b = m_mesh->indices ? m_mesh->indices[3 * t + 1] : 3 * t + 1;
					const int c = m_mesh->indices ? m_mesh->indices[3 * t + 2] : 3 * t + 2;
					m_impl->triangles->addTriangle(vertex(a), vertex(b), vertex(c));
				}
				m_impl->shape = std::make_unique<btBvhTriangleMeshShape>(m_impl->triangles.get(), true);
				const BoundingBox bounds = GetMeshBoundingBox(*m_mesh);
				halfHeight = 0.5f * (bounds.max.y - bounds.min.y);
				break;
			}
		}

		if (!m_impl->shape)
			m_impl->shape = std::make_unique<btSphereShape>(0.1f);	// placeholder so bodies stay valid

		if (m_isBottomPivot)
			m_center = { 0.0f, halfHeight, 0.0f };

		const bool hasOffset = std::fabs(m_center.x) + std::fabs(m_center.y) + std::fabs(m_center.z) > 1e-6f;
		if (hasOffset)
		{
			if (m_shape == ColliderShape::StaticMesh)
				Log::Warning("Collider: center offset ignored for static meshes");
			else
			{
				m_impl->compound = std::make_unique<btCompoundShape>();
				m_impl->compound->addChildShape(btTransform(btQuaternion::getIdentity(), bt::ToBt(m_center)), m_impl->shape.get());
			}
		}

		m_impl->Root()->setLocalScaling(bt::ToBt(m_scale));
	}

	void Collider::DrawInspector(InspectorLayout& ui)
	{
		static constexpr const char*	SHAPE_NAMES[]	= { "Box", "Sphere", "Capsule", "Convex hull", "Static mesh" };
		static constexpr const char*	LAYER_NAMES[]	= { "Ground", "Prop", "Player", "Trigger", "Beam" };
		static constexpr uint32_t		LAYER_VALUES[]	= { Layer::Ground, Layer::Prop, Layer::Player, Layer::Trigger, Layer::Beam };

		bool shapeChanged = false;
		ui.Info("Shape", SHAPE_NAMES[static_cast<int>(m_shape)]);
		switch (m_shape)
		{
			case ColliderShape::Box:
			{
				Vector3 half = m_halfExtents;
				if (ui.Vec3("Half extents", half))
				{
					SetBox({ std::max(half.x, 0.01f), std::max(half.y, 0.01f), std::max(half.z, 0.01f) });
					shapeChanged = true;
				}
				break;
			}
			case ColliderShape::Sphere:
			{
				float radius = m_radius;
				if (ui.Float("Radius", radius))	{ SetSphere(std::max(radius, 0.01f)); shapeChanged = true; }
				break;
			}
			case ColliderShape::Capsule:
			{
				float radius = m_radius, height = m_height;
				const bool r = ui.Float("Radius", radius);
				const bool h = ui.Float("Height", height);
				if (r || h)	{ SetCapsule(std::max(radius, 0.01f), std::max(height, 0.0f)); shapeChanged = true; }
				break;
			}
			default:
				break;	// meshes are edited as assets, not here
		}

		Vector3 center = m_center;
		if (ui.Vec3("Center", center))	{ SetCenter(center); shapeChanged = true; }

		int layer = 0;
		for (int i = 0; i < 5; ++i)
			if (LAYER_VALUES[i] == m_layer)
				layer = i;
		if (ui.Choice("Layer", LAYER_NAMES, 5, layer))
		{
			SetLayer(LAYER_VALUES[layer]);
			shapeChanged = true;
		}

		if (shapeChanged)
			OnFieldsChanged();
	}

	void Collider::OnFieldsChanged()
	{
		// The body / ghost picks up a new shape or filter when it is re-created
		if (GameActor* owner = GetOwner())
		{
			if (RigidBody* body = owner->GetComponent<RigidBody>())			{ body->SetEnabled(false); body->SetEnabled(true); }
			if (TriggerVolume* trigger = owner->GetComponent<TriggerVolume>())	{ trigger->SetEnabled(false); trigger->SetEnabled(true); }
		}
	}

	namespace
	{
		constexpr uint32_t LAYERS[] = { Layer::Ground, Layer::Prop, Layer::Player, Layer::Trigger, Layer::Beam };
	}

	void Collider::Serialize(PropertyBag& out) const
	{
		switch (m_shape)
		{
			case ColliderShape::Box:		out.SetString("shape", "box");		out.SetVec3("half", m_halfExtents); break;
			case ColliderShape::Sphere:		out.SetString("shape", "sphere");	out.SetFloat("radius", m_radius); break;
			case ColliderShape::Capsule:	out.SetString("shape", "capsule");	out.SetFloat("radius", m_radius); out.SetFloat("height", m_height); break;
			default:
				Log::Warning("Collider: mesh shapes are not saved in level files; '%s' will load with a box",
					GetOwner() ? GetOwner()->GetName().c_str() : "?");
				out.SetString("shape", "box");
				break;
		}

		if (m_isBottomPivot)
			out.SetBool("bottomPivot", true);
		else if (m_center.x != 0.0f || m_center.y != 0.0f || m_center.z != 0.0f)
			out.SetVec3("center", m_center);

		out.SetString("layer", LayerName(m_layer));
		if (m_mask != DefaultCollisionMask(m_layer))
			out.SetInt("mask", static_cast<int>(m_mask));
	}

	void Collider::Deserialize(const PropertyBag& in)
	{
		const std::string shape = in.GetString("shape", "box");
		if (shape == "sphere")			SetSphere(in.GetFloat("radius", 0.5f));
		else if (shape == "capsule")	SetCapsule(in.GetFloat("radius", 0.5f), in.GetFloat("height", 1.0f));
		else							SetBox(in.GetVec3("half", { 0.5f, 0.5f, 0.5f }));

		if (in.Has("center"))			SetCenter(in.GetVec3("center"));
		if (in.GetBool("bottomPivot"))	SetBottomPivot(true);

		const std::string layer = in.GetString("layer", "Prop");
		for (uint32_t candidate : LAYERS)
			if (layer == LayerName(candidate))
				SetLayer(candidate);
		if (in.Has("mask"))
			SetMask(static_cast<uint32_t>(in.GetInt("mask")));
	}
}
