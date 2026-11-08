#pragma once
#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eng
{
	/// <summary>
	/// Typed publish / subscribe, owned by the Engine. Any copyable struct is an event:
	///
	///   struct GameWon { int level; };
	///   auto token = events.Subscribe<GameWon>([](const GameWon& e) { ... });
	///   events.Publish(GameWon{ 2 });     // queued
	///   events.Unsubscribe(token);
	///
	/// Published events are queued and delivered in order by Dispatch() at the end of the frame (so
	/// handlers never run in the middle of a physics step or a level update). Events published while
	/// dispatching are delivered on the next Dispatch(). A handler removed during dispatch is not called again.
	/// </summary>
	class EventBus
	{
	public:
		using Token = uint64_t;

		template<typename E>
		Token Subscribe(std::function<void(const E&)> handler)
		{
			const Token token = ++m_lastToken;
			m_handlers[TypeId<E>()].push_back({ token, [handler = std::move(handler)](const std::any& event)
			{
				handler(std::any_cast<const E&>(event));
			} });
			return token;
		}

		void	Unsubscribe(Token token);

		template<typename E>
		void Publish(const E& event)
		{
			m_queue.push_back({ TypeId<E>(), std::any(event) });
		}

		/// <summary>
		/// Delivers everything queued before this call. Called by the Engine at the end of each frame.
		/// </summary>
		void	Dispatch();

		size_t	GetPendingCount() const;
		size_t	GetHandlerCount() const;
		void	Clear();	// drops handlers and pending events

	private:
		using TypeKey = size_t;

		// One address per event type: a type id without RTTI
		template<typename E>
		static TypeKey TypeId()
		{
			static const char marker = 0;
			return reinterpret_cast<TypeKey>(&marker);
		}

		struct Handler
		{
			Token									token;
			std::function<void(const std::any&)>	call;
		};

		struct Pending
		{
			TypeKey		type;
			std::any	event;
		};

		std::unordered_map<TypeKey, std::vector<Handler>>	m_handlers;
		std::vector<Pending>								m_queue;
		std::vector<Token>									m_removedDuringDispatch;
		bool												m_isDispatching	= false;
		Token												m_lastToken		= 0;
	};
}
