#include "EnemyTypes.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace enemies
{
	namespace
	{
		// Plan §8.1. The loseRadius and attackExitRange values are 1.2 x the trigger and attack range (E-D6).
		constexpr std::array<EnemyArchetype, static_cast<size_t>(EnemyType::Count)> ARCHETYPES = { {
			//  name       body  bodyColour                 spikeColour                hp   wander chase turn  trig  lose  atk   exit  dmg avoid
			{ "drifter",  0.45f, Color{ 70, 190, 90, 255 }, Color{ 170, 240, 160, 255 }, 30.0f, 1.5f, 3.0f, 90.0f, 7.0f, 8.4f, 1.6f, 1.92f, 5.0f, 1.0f },
			{ "stalker",  0.55f, Color{ 240, 140, 30, 255 }, Color{ 250, 230, 80, 255 }, 60.0f, 2.5f, 5.0f, 180.0f, 10.0f, 12.0f, 1.8f, 2.16f, 10.0f, 1.5f },
			{ "brute",    0.80f, Color{ 200, 40, 40, 255 }, Color{ 110, 20, 20, 255 },  120.0f, 2.0f, 7.0f, 270.0f, 14.0f, 16.8f, 2.3f, 2.76f, 20.0f, 0.5f },
		} };
	}

	const EnemyArchetype& GetArchetype(EnemyType type)
	{
		return ARCHETYPES[static_cast<size_t>(type)];
	}

	bool ParseEnemyType(const std::string& name, EnemyType& type)
	{
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		for (size_t i = 0; i < ARCHETYPES.size(); ++i)
		{
			if (lower == ARCHETYPES[i].name)
			{
				type = static_cast<EnemyType>(i);
				return true;
			}
		}
		return false;
	}
}
