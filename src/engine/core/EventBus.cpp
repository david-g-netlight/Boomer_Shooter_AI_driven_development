#include "EventBus.h"
#include <algorithm>

namespace eng
{
	void EventBus::Unsubscribe(Token token)
	{
		for (auto& [type, handlers] : m_handlers)
		{
			auto it = std::find_if(handlers.begin(), handlers.end(), [token](const Handler& h) { return h.token == token; });
			if (it == handlers.end())
				continue;

			if (m_isDispatching)
				m_removedDuringDispatch.push_back(token);	// erased after dispatch; skipped from now on
			else
				handlers.erase(it);
			return;
		}
	}

	void EventBus::Dispatch()
	{
		if (m_queue.empty())
			return;

		// Take the queue: events published by handlers wait for the next Dispatch()
		std::vector<Pending> events;
		events.swap(m_queue);

		m_isDispatching = true;
		for (const Pending& pending : events)
		{
			auto it = m_handlers.find(pending.type);
			if (it == m_handlers.end())
				continue;

			// A reference, not the iterator: subscribing to a new event type may rehash the map, which
			// invalidates iterators but not references to its elements. Index loop: subscribing to this
			// type may grow (reallocate) the vector.
			std::vector<Handler>& handlers = it->second;
			for (size_t i = 0; i < handlers.size(); ++i)
			{
				const Token token = handlers[i].token;
				if (std::find(m_removedDuringDispatch.begin(), m_removedDuringDispatch.end(), token) != m_removedDuringDispatch.end())
					continue;
				// Copy: the handler may change the vector while it runs
				const auto call = handlers[i].call;
				call(pending.event);
			}
		}
		m_isDispatching = false;

		for (Token token : m_removedDuringDispatch)
			Unsubscribe(token);
		m_removedDuringDispatch.clear();
	}

	size_t EventBus::GetPendingCount() const
	{
		return m_queue.size();
	}

	size_t EventBus::GetHandlerCount() const
	{
		size_t count = 0;
		for (const auto& [type, handlers] : m_handlers)
			count += handlers.size();
		return count;
	}

	void EventBus::Clear()
	{
		m_handlers.clear();
		m_queue.clear();
		m_removedDuringDispatch.clear();
	}
}
