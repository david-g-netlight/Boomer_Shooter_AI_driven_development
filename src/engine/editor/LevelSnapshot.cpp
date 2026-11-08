#include "LevelSnapshot.h"
#include <unordered_map>
#include "level/GameActor.h"
#include "level/Level.h"
#include "physics/CharacterMotor.h"
#include "physics/RigidBody.h"

namespace eng
{
	void LevelSnapshot::Capture(const Level& level)
	{
		m_states.clear();
		m_states.reserve(level.GetActorCount());

		for (const auto& owned : level.GetActors())
		{
			const GameActor& actor = *owned;
			if (actor.IsMarkedForDestroy())
				continue;

			ActorState state;
			state.actor			= &actor;
			state.name			= actor.GetName();
			state.isActive		= actor.IsActive();
			state.transform		= actor.GetTransform();
			state.hasMotor		= actor.GetComponent<CharacterMotor>() != nullptr;

			if (const RigidBody* body = actor.GetComponent<RigidBody>())
			{
				state.hasBody = body->HasBody() && body->GetType() == BodyType::Dynamic;
				if (state.hasBody)
				{
					state.linearVelocity	= body->GetLinearVelocity();
					state.angularVelocity	= body->GetAngularVelocity();
				}
			}
			m_states.push_back(std::move(state));
		}
		m_hasCapture = true;
	}

	LevelSnapshot::RestoreResult LevelSnapshot::Restore(Level& level) const
	{
		RestoreResult result;
		if (!m_hasCapture)
			return result;

		std::unordered_map<const GameActor*, const ActorState*> byActor;
		byActor.reserve(m_states.size());
		for (const ActorState& state : m_states)
			byActor.emplace(state.actor, &state);

		for (const auto& owned : level.GetActors())
		{
			GameActor& actor = *owned;
			if (actor.IsMarkedForDestroy())
				continue;

			const auto found = byActor.find(&actor);
			if (found == byActor.end() || found->second->name != actor.GetName())
			{
				// Created in the editor (a recycled address with another name counts as new)
				actor.MarkForDestroy();
				++result.removed;
				continue;
			}

			const ActorState& state = *found->second;
			actor.SetActive(state.isActive);

			if (RigidBody* body = actor.GetComponent<RigidBody>())
			{
				body->Teleport(state.transform);	// pose + zero velocity
				body->OnScaleChanged();
				if (state.hasBody)
				{
					body->SetLinearVelocity(state.linearVelocity);
					body->SetAngularVelocity(state.angularVelocity);
				}
			}
			else if (CharacterMotor* motor = actor.GetComponent<CharacterMotor>(); motor && state.hasMotor)
			{
				actor.GetTransform() = state.transform;
				motor->Warp(state.transform.translation);
			}
			else
			{
				actor.GetTransform() = state.transform;
			}
			byActor.erase(found);
			++result.restored;
		}

		result.missing = static_cast<int>(byActor.size());
		return result;
	}

	bool LevelSnapshot::IsEmpty() const			{ return !m_hasCapture; }
	size_t LevelSnapshot::GetActorCount() const	{ return m_states.size(); }

	void LevelSnapshot::Clear()
	{
		m_states.clear();
		m_hasCapture = false;
	}
}
