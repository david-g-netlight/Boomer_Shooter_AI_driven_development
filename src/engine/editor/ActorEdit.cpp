#include "ActorEdit.h"
#include "level/GameActor.h"
#include "physics/CharacterMotor.h"
#include "physics/Collider.h"
#include "physics/RigidBody.h"

namespace eng::ActorEdit
{
	void SetLocalTransform(GameActor& actor, const ::Transform& transform)
	{
		if (RigidBody* body = actor.GetComponent<RigidBody>())
		{
			body->Teleport(transform);
			body->OnScaleChanged();
		}
		else if (CharacterMotor* motor = actor.GetComponent<CharacterMotor>())
		{
			actor.GetTransform() = transform;
			motor->Warp(transform.translation);
		}
		else
		{
			actor.GetTransform() = transform;
			if (Collider* collider = actor.GetComponent<Collider>())
				collider->SyncScaleFromActor();
		}
	}
}
