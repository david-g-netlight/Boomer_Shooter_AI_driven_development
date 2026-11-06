#include "ActorMotionState.h"
#include "level/GameActor.h"
#include <raymath.h>
#include "core/MathUtils.h"

namespace eng
{
	ActorMotionState::ActorMotionState(GameActor& actor, bool writeRotation)
		: m_actor(actor)
		, m_writeRotation(writeRotation)
	{
	}

	void ActorMotionState::getWorldTransform(btTransform& worldTransform) const
	{
		Vector3		position;
		Quaternion	rotation;
		Vector3		scale;
		DecomposeTransform(m_actor.GetWorldMatrix(), &position, &rotation, &scale);
		worldTransform = btTransform(bt::ToBt(rotation), bt::ToBt(position));
	}

	void ActorMotionState::setWorldTransform(const btTransform& worldTransform)
	{
		const Vector3		worldPosition	= bt::ToRaylib(worldTransform.getOrigin());
		const Quaternion	worldRotation	= bt::ToRaylibQuaternion(worldTransform.getRotation());
		::Transform&		local			= m_actor.GetTransform();

		const GameActor* parent = m_actor.GetParent();
		if (!parent)
		{
			local.translation = worldPosition;
			if (m_writeRotation)
				local.rotation = worldRotation;
			return;
		}

		// Into the parent's space: position through the inverse parent matrix, rotation relative to the parent's
		const Matrix parentWorld = parent->GetWorldMatrix();
		local.translation = Vector3Transform(worldPosition, MatrixInvert(parentWorld));
		if (m_writeRotation)
		{
			Vector3		parentPosition;
			Quaternion	parentRotation;
			Vector3		parentScale;
			DecomposeTransform(parentWorld, &parentPosition, &parentRotation, &parentScale);
			local.rotation = QuaternionNormalize(QuaternionMultiply(QuaternionInvert(parentRotation), worldRotation));
		}
	}
}
