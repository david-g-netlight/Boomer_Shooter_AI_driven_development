#pragma once
#include <concepts>
#include <functional>
#include <string>
#include <unordered_map>
#include "level/GameActor.h"

namespace eng
{
	/// <summary>
	/// Component type name -> factory, so level files can create components by name. The Engine registers
	/// its own components; the game registers its own in Application::Init():
	///
	///		Engine::GetInstance().GetComponentRegistry().Register<TriggerCounter>("TriggerCounter");
	///
	/// The name must equal what the component's TypeName() returns. Needs a default constructor.
	/// </summary>
	class ComponentRegistry
	{
	public:
		template<typename T>
		requires std::derived_from<T, Component> && std::default_initializable<T>
		void Register(const std::string& typeName)
		{
			m_factories[typeName] = [](GameActor& actor) -> Component* { return actor.AddComponent<T>(); };
		}

		bool		IsRegistered(const std::string& typeName) const;

		/// <summary>
		/// Adds a component of that type to `actor`; nullptr if the type is unknown.
		/// </summary>
		Component*	Create(const std::string& typeName, GameActor& actor) const;

	private:
		std::unordered_map<std::string, std::function<Component*(GameActor&)>>	m_factories;
	};
}
