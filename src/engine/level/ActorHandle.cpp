#include "ActorHandle.h"
#include "level/GameActor.h"
#include "level/Level.h"

namespace eng
{
	ActorHandle::ActorHandle(const GameActor* actor)
		: m_id(actor ? actor->GetId() : INVALID_ACTOR_ID)
	{
	}

	GameActor* ActorHandle::Get() const
	{
		return m_id == INVALID_ACTOR_ID ? nullptr : Level::FindActor(m_id);
	}

	bool ActorHandle::IsValid() const	{ return Get() != nullptr; }

	bool ActorHandle::IsAlive() const
	{
		const GameActor* actor = Get();
		return actor && !actor->IsMarkedForDestroy();
	}

	ActorId ActorHandle::GetId() const	{ return m_id; }
	void ActorHandle::Reset()			{ m_id = INVALID_ACTOR_ID; }
}
