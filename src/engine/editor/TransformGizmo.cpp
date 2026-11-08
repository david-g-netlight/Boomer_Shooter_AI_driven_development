#include "TransformGizmo.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>
#include "level/GameActor.h"
#include "physics/Collider.h"
#include "physics/CharacterMotor.h"
#include "physics/RigidBody.h"
#include "render/Camera.h"

namespace eng
{
	namespace
	{
		constexpr Color AXIS_COLORS[3] = { { 230, 60, 60, 255 }, { 70, 200, 70, 255 }, { 70, 110, 240, 255 } };
		constexpr Color ACTIVE_COLOR	= { 255, 230, 60, 255 };

		/// <summary>
		/// Closest points between the infinite ray (p1 + t d1) and axis line (p2 + s d2).
		/// False when they are parallel.
		/// </summary>
		bool ClosestRayAxis(const Vector3& p1, const Vector3& d1, const Vector3& p2, const Vector3& d2, float& t, float& s)
		{
			const Vector3 w		= Vector3Subtract(p1, p2);
			const float a		= Vector3DotProduct(d1, d1);
			const float b		= Vector3DotProduct(d1, d2);
			const float c		= Vector3DotProduct(d2, d2);
			const float d		= Vector3DotProduct(d1, w);
			const float e		= Vector3DotProduct(d2, w);
			const float denom	= a * c - b * b;
			if (std::fabs(denom) < 1e-6f)
				return false;

			t = (b * e - c * d) / denom;
			s = (a * e - b * d) / denom;
			return true;
		}
	}

	TransformGizmo::AxisFrame TransformGizmo::MakeFrame(const Camera& camera, const GameActor& target) const
	{
		AxisFrame frame;
		frame.origin	= target.GetWorldPosition();
		frame.length	= std::max(0.05f, Vector3Distance(camera.GetPosition(), frame.origin) * HANDLE_SCREEN_SIZE);

		if (m_mode == GizmoMode::Move)
		{
			frame.dir[0] = { 1.0f, 0.0f, 0.0f };
			frame.dir[1] = { 0.0f, 1.0f, 0.0f };
			frame.dir[2] = { 0.0f, 0.0f, 1.0f };
		}
		else
		{
			const Matrix m = target.GetWorldMatrix();	// basis columns are the actor's axes (scaled)
			frame.dir[0] = Vector3Normalize({ m.m0, m.m1, m.m2 });
			frame.dir[1] = Vector3Normalize({ m.m4, m.m5, m.m6 });
			frame.dir[2] = Vector3Normalize({ m.m8, m.m9, m.m10 });
		}
		return frame;
	}

	int TransformGizmo::HitTest(const Ray& ray, const AxisFrame& frame) const
	{
		int		best		= NO_AXIS;
		float	bestT		= FLT_MAX;
		for (int axis = 0; axis < 3; ++axis)
		{
			float t, s;
			if (!ClosestRayAxis(ray.position, ray.direction, frame.origin, frame.dir[axis], t, s))
				continue;
			if (t < 0.0f || s < -PICK_RADIUS * frame.length || s > frame.length * 1.1f)
				continue;

			const Vector3 onRay	 = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
			const Vector3 onAxis = Vector3Add(frame.origin, Vector3Scale(frame.dir[axis], s));
			if (Vector3Distance(onRay, onAxis) <= PICK_RADIUS * frame.length && t < bestT)
			{
				best	= axis;
				bestT	= t;
			}
		}
		return best;
	}

	bool TransformGizmo::Update(const Camera& camera, const Vector2& mouse, bool leftPressed, bool leftDown, GameActor* target)
	{
		if (!target || target->IsMarkedForDestroy())
		{
			CancelDrag();
			m_hoverAxis = NO_AXIS;
			return false;
		}

		const Ray ray = GetScreenToWorldRay(mouse, camera.ToRaylib());

		if (m_dragAxis != NO_AXIS)
		{
			if (!leftDown)
			{
				CancelDrag();
				return true;	// the release belongs to the gizmo
			}

			float t, s;
			if (ClosestRayAxis(ray.position, ray.direction, m_dragFrame.origin, m_dragFrame.dir[m_dragAxis], t, s))
			{
				const float delta = s - m_dragStartT;
				if (m_mode == GizmoMode::Move)
				{
					Apply(*target, Vector3Add(m_startWorldPos, Vector3Scale(m_dragFrame.dir[m_dragAxis], delta)), m_startScale);
				}
				else
				{
					Vector3 scale = m_startScale;
					float* component = m_dragAxis == 0 ? &scale.x : (m_dragAxis == 1 ? &scale.y : &scale.z);
					*component = std::max(MIN_SCALE, *component * (1.0f + delta / m_dragFrame.length));
					Apply(*target, m_startWorldPos, scale);
				}
			}
			return true;
		}

		const AxisFrame frame = MakeFrame(camera, *target);
		m_hoverAxis = HitTest(ray, frame);

		if (m_hoverAxis != NO_AXIS && leftPressed)
		{
			float t, s;
			if (ClosestRayAxis(ray.position, ray.direction, frame.origin, frame.dir[m_hoverAxis], t, s))
			{
				m_dragAxis		= m_hoverAxis;
				m_dragFrame		= frame;
				m_dragStartT	= s;
				m_startWorldPos	= frame.origin;
				m_startScale	= target->GetTransform().scale;
			}
		}
		return m_hoverAxis != NO_AXIS;
	}

	void TransformGizmo::Apply(GameActor& target, const Vector3& worldPosition, const Vector3& scale) const
	{
		// World position -> the actor's local space
		Vector3 local = worldPosition;
		if (const GameActor* parent = target.GetParent())
			local = Vector3Transform(worldPosition, MatrixInvert(parent->GetWorldMatrix()));

		::Transform transform	= target.GetTransform();
		transform.translation	= local;
		transform.scale			= scale;

		if (RigidBody* body = target.GetComponent<RigidBody>())
		{
			body->Teleport(transform);	// pose + zero velocity
			body->OnScaleChanged();
		}
		else if (CharacterMotor* motor = target.GetComponent<CharacterMotor>())
		{
			target.GetTransform() = transform;
			motor->Warp(local);
		}
		else
		{
			target.GetTransform() = transform;
			if (Collider* collider = target.GetComponent<Collider>())
				collider->SyncScaleFromActor();
		}
	}

	void TransformGizmo::Draw3D(const Camera& camera, const GameActor* target) const
	{
		if (!target || target->IsMarkedForDestroy())
			return;

		const AxisFrame frame = MakeFrame(camera, *target);
		const float shaft = frame.length * 0.012f;

		rlDrawRenderBatchActive();
		rlDisableDepthTest();	// handles stay visible through geometry
		for (int axis = 0; axis < 3; ++axis)
		{
			const bool	active	= axis == m_dragAxis || (m_dragAxis == NO_AXIS && axis == m_hoverAxis);
			const Color	color	= active ? ACTIVE_COLOR : AXIS_COLORS[axis];
			const Vector3 tip	= Vector3Add(frame.origin, Vector3Scale(frame.dir[axis], frame.length));

			if (m_mode == GizmoMode::Move)
			{
				const Vector3 neck = Vector3Add(frame.origin, Vector3Scale(frame.dir[axis], frame.length * 0.8f));
				DrawCylinderEx(frame.origin, neck, shaft, shaft, 6, color);
				DrawCylinderEx(neck, tip, shaft * 3.0f, 0.0f, 8, color);
			}
			else
			{
				DrawCylinderEx(frame.origin, tip, shaft, shaft, 6, color);
				DrawCube(tip, frame.length * 0.09f, frame.length * 0.09f, frame.length * 0.09f, color);
			}
		}
		rlDrawRenderBatchActive();
		rlEnableDepthTest();
	}

	void TransformGizmo::SetMode(GizmoMode mode)	{ CancelDrag(); m_mode = mode; }
	GizmoMode TransformGizmo::GetMode() const		{ return m_mode; }
	bool TransformGizmo::IsDragging() const			{ return m_dragAxis != NO_AXIS; }
	void TransformGizmo::CancelDrag()				{ m_dragAxis = NO_AXIS; }
}
