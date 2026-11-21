#pragma once
#include <cstdint>

namespace eng::ecs
{
	/// <summary>
	/// 4 bytes: a slot index and how many times that slot was reused.
	/// A stale id (the slot was released and taken again) has an old generation and is rejected by the Registry.
	/// </summary>
	struct Entity
	{
		static constexpr uint16_t INVALID_INDEX = 0xFFFF;

		uint16_t	index		= INVALID_INDEX;
		uint16_t	generation	= 0;

		bool operator == (const Entity&) const = default;
	};

	inline constexpr Entity INVALID_ENTITY{};
}
