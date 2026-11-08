#include "Level.h"
#include "core/Log.h"
#include <algorithm>
#include <unordered_map>

namespace eng
{
	namespace
	{
		// Every live actor of every Level. Ids start at 1 and are never reused.
		// Never destroyed on purpose: Levels owned by the Engine singleton are deleted during static destruction
		std::unordered_map<ActorId, GameActor*>&	Registry()
		{
			static auto* registry = new std::unordered_map<ActorId, GameActor*>();
			return *registry;
		}

		ActorId s_nextId = 1;
	}

	struct Level::PlainActor : GameActor
	{
		PlainActor() = default;
	};

	Level::~Level()
	{
		Clear();
	}

	//////////////////////////////////////////
	/// Factory
	//////////////////////////////////////////
	GameActor* Level::CreateGameActor(const std::string& name, GameActor* parent)
	{
		auto actor		= std::make_unique<PlainActor>();
		GameActor* raw	= actor.get();
		Register(std::move(actor), name, parent);
		return raw;
	}

	void Level::Register(std::unique_ptr<GameActor> actor, const std::string& name, GameActor* parent)
	{
		actor->SetName(name);
		actor->m_level	= this;
		actor->m_id		= s_nextId++;
		Registry()[actor->m_id] = actor.get();
		if (parent)
			SetParent(actor.get(), parent);
		m_actors.push_back(std::move(actor));
	}

	bool Level::SetParent(GameActor* actor, GameActor* parent, bool keepWorldTransform)
	{
		if (!actor || actor->m_level != this || (parent && parent->m_level != this))
		{
			Log::Warning("Level::SetParent: both actors must belong to this level");
			return false;
		}
		return actor->SetParent(parent, keepWorldTransform);
	}

	//////////////////////////////////////////
	/// Per frame
	//////////////////////////////////////////
	void Level::Update(float deltaTime)
	{
		// By index over a snapshot of the count: actors created during the loop start next frame,
		// and m_actors growing (reallocating) can't invalidate the loop
		const size_t count = m_actors.size();
		for (size_t i = 0; i < count; ++i)
		{
			GameActor* actor = m_actors[i].get();
			if (!actor->IsMarkedForDestroy() && actor->IsActiveInHierarchy())
				actor->Update(deltaTime);
		}
	}

	void Level::FixedUpdate(float fixedDeltaTime)
	{
		const size_t count = m_actors.size();
		for (size_t i = 0; i < count; ++i)
		{
			GameActor* actor = m_actors[i].get();
			if (!actor->IsMarkedForDestroy() && actor->IsActiveInHierarchy())
				actor->FixedUpdate(fixedDeltaTime);
		}
	}

	void Level::SubmitRenderables(RenderQueue& queue) const
	{
		for (const auto& actor : m_actors)
			if (!actor->IsMarkedForDestroy() && actor->IsActiveInHierarchy())
				actor->SubmitRenderables(queue);
	}

	void Level::FlushDestroyed()
	{
		const bool anyMarked = std::any_of(m_actors.begin(), m_actors.end(),
			[](const auto& actor) { return actor->IsMarkedForDestroy(); });
		if (!anyMarked)
			return;

		// 1. Unlink every marked actor first, so no destructor sees a parent or child that is already gone.
		//    Children of marked actors are marked too (MarkForDestroy is recursive).
		for (const auto& actor : m_actors)
		{
			if (!actor->IsMarkedForDestroy())
				continue;
			actor->DetachFromParent();
			for (GameActor* child : actor->m_children)
				child->m_parent = nullptr;
			actor->m_children.clear();
		}

		// 2. Handles stop resolving, then delete (components get OnDisable / OnDestroy)
		for (const auto& actor : m_actors)
			if (actor->IsMarkedForDestroy())
				Registry().erase(actor->m_id);

		m_actors.erase(std::remove_if(m_actors.begin(), m_actors.end(),
			[](const auto& actor) { return actor->IsMarkedForDestroy(); }), m_actors.end());
	}

	//////////////////////////////////////////
	/// Queries
	//////////////////////////////////////////
	GameActor* Level::FindByName(const std::string& name) const
	{
		for (const auto& actor : m_actors)
			if (!actor->IsMarkedForDestroy() && actor->GetName() == name)
				return actor.get();
		return nullptr;
	}

	GameActor* Level::FindActor(ActorId id)
	{
		const auto it = Registry().find(id);
		return it == Registry().end() ? nullptr : it->second;
	}

	size_t Level::GetActorCount() const
	{
		return m_actors.size();
	}

	const std::vector<std::unique_ptr<GameActor>>& Level::GetActors() const
	{
		return m_actors;
	}

	const SpawnPoint& Level::GetSpawn() const
	{
		return m_spawn;
	}

	void Level::SetSpawn(const Vector3& feetPosition, float yawDegrees)
	{
		m_spawn = { true, feetPosition, yawDegrees };
	}

	void Level::Clear()
	{
		m_spawn = {};
		for (const auto& actor : m_actors)
			actor->MarkForDestroy();
		FlushDestroyed();
	}
}
