#pragma once
#include <cstdint>

namespace eng
{
	class GameActor;

	using ActorId = uint32_t;
	constexpr ActorId INVALID_ACTOR_ID = 0;

	/// <summary>
	/// A weak reference to a GameActor: the actor's id, never its address. Get() looks the actor up and returns
	/// nullptr once it was deleted (or its Level was cleared / destroyed), so game code can keep a handle for as
	/// long as it likes without a dangling pointer. Cheap to copy and compare; safe to hold across frames,
	/// level reloads and the editor's Load.
	///
	/// An actor that is marked for destroy still resolves until the Level deletes it at the end of the frame
	/// (check GameActor::IsMarkedForDestroy() or use IsAlive() when that matters).
	/// Don't keep the raw pointer Get() returns past the current frame.
	/// </summary>
	class ActorHandle
	{
	public:
		ActorHandle() = default;
		ActorHandle(const GameActor* actor);	// implicit: handle = actor

		GameActor*	Get() const;
		bool		IsValid() const;			// the actor still exists
		bool		IsAlive() const;			// exists and is not marked for destroy
		ActorId		GetId() const;
		void		Reset();

		explicit operator bool() const	{ return IsValid(); }
		GameActor*	operator->() const	{ return Get(); }
		GameActor&	operator*() const	{ return *Get(); }

		friend bool operator==(const ActorHandle& a, const ActorHandle& b) { return a.m_id == b.m_id; }
		friend bool operator!=(const ActorHandle& a, const ActorHandle& b) { return a.m_id != b.m_id; }

	private:
		ActorId		m_id	= INVALID_ACTOR_ID;
	};
}
