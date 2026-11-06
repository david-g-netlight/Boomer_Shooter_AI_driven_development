#pragma once
#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "GameActor.h"

namespace eng
{
	class RenderQueue;

	/// <summary>
	/// Owns the GameActors of one scene and drives them each frame. The Engine updates the
	/// Application's active level (Application::GetActiveLevel()) before the Application itself.
	///
	/// Actors created during an update start updating next frame. Actors marked for destroy are skipped
	/// immediately and deleted in FlushDestroyed() at the end of the frame.
	/// </summary>
	class Level
	{
	public:
		Level() = default;
		~Level();

		Level(const Level&) = delete;
		Level& operator = (const Level&) = delete;

		//////////////////////////////////////////
		/// Factory
		//////////////////////////////////////////
		GameActor*	CreateGameActor(const std::string& name, GameActor* parent = nullptr);

		template<typename T>
		requires std::derived_from<T, GameActor>
		T* CreateGameActor(const std::string& name, GameActor* parent = nullptr)
		{
			auto actor	= std::make_unique<T>();
			T* raw		= actor.get();
			Register(std::move(actor), name, parent);
			return raw;
		}

		/// <summary>
		/// Both actors must belong to this level (parent may be nullptr = root).
		/// </summary>
		bool		SetParent(GameActor* actor, GameActor* parent, bool keepWorldTransform = false);

		//////////////////////////////////////////
		/// Per frame (called by the Engine)
		//////////////////////////////////////////
		void		Update(float deltaTime);
		void		FixedUpdate(float fixedDeltaTime);
		void		SubmitRenderables(RenderQueue& queue) const;
		void		FlushDestroyed();

		//////////////////////////////////////////
		/// Queries
		//////////////////////////////////////////
		GameActor*	FindByName(const std::string& name) const;
		size_t		GetActorCount() const;
		const std::vector<std::unique_ptr<GameActor>>& GetActors() const;

		/// <summary>
		/// Deletes every actor now. Don't call from inside an Update (use MarkForDestroy there).
		/// </summary>
		void		Clear();

	private:
		/// <summary>
		/// Plain GameActor has a protected constructor; the Level is a friend and makes them here.
		/// </summary>
		struct PlainActor;

		void		Register(std::unique_ptr<GameActor> actor, const std::string& name, GameActor* parent);

		std::vector<std::unique_ptr<GameActor>>	m_actors;
	};
}
