#include "BulletConvert.h"
#include "Collider.h"
#include "level/GameActor.h"
#include "core/Log.h"
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <raymath.h>

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
		MatrixDecompose(world, &pose.translation, &pose.rotation, &scale);

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
}
