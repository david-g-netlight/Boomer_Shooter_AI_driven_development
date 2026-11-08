#include "ComponentRegistry.h"
#include <algorithm>

namespace eng
{
	bool ComponentRegistry::IsRegistered(const std::string& typeName) const
	{
		return m_factories.contains(typeName);
	}

	std::vector<std::string> ComponentRegistry::GetTypeNames() const
	{
		std::vector<std::string> names;
		names.reserve(m_factories.size());
		for (const auto& entry : m_factories)
			names.push_back(entry.first);
		std::sort(names.begin(), names.end());
		return names;
	}

	Component* ComponentRegistry::Create(const std::string& typeName, GameActor& actor) const
	{
		const auto it = m_factories.find(typeName);
		return it == m_factories.end() ? nullptr : it->second(actor);
	}
}
