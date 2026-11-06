#include "Level.h"
#include "core/Log.h"
#include <algorithm>

namespace eng
{
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
		actor->m_level = this;
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

		// 2. Delete (components get OnDisable / OnDestroy)
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

	size_t Level::GetActorCount() const
	{
		return m_actors.size();
	}

	const std::vector<std::unique_ptr<GameActor>>& Level::GetActors() const
	{
		return m_actors;
	}

	void Level::Clear()
	{
		for (const auto& actor : m_actors)
			actor->MarkForDestroy();
		FlushDestroyed();
	}
}
