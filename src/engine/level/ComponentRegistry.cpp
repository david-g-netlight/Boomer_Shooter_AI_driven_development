#include "ComponentRegistry.h"

namespace eng
{
	bool ComponentRegistry::IsRegistered(const std::string& typeName) const
	{
		return m_factories.contains(typeName);
	}

	Component* ComponentRegistry::Create(const std::string& typeName, GameActor& actor) const
	{
		const auto it = m_factories.find(typeName);
		return it == m_factories.end() ? nullptr : it->second(actor);
	}
}
