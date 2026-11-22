#pragma once
#include <raylib.h>
#include <cstdint>
#include <string>

namespace enemies
{
	enum class EnemyType : uint8_t
	{
		Drifter,
		Stalker,
		Brute,
		Count,
	};

	/// <summary>
	/// The tuning of one enemy type. Its values are copied into the components at spawn (E-D8), so a console
	/// change only affects enemies spawned after it.
	/// </summary>
	struct EnemyArchetype
	{
		const char*	name;
		float		bodyRadius;
		Color		bodyColor;
		Color		spikeColor;
		float		hp;
		float		wanderSpeed;		// m/s
		float		chaseSpeed;			// m/s
		float		turnRateDeg;		// degrees per second
		float		triggerRadius;		// m, chase starts inside it
		float		loseRadius;			// m, chase ends beyond it (1.2 x trigger, E-D6)
		float		attackRange;		// m, centre to chest
		float		attackExitRange;	// m, attack ends beyond it (1.2 x range)
		float		damage;				// per hit, 1 s cooldown
		float		avoidStrength;
	};

	/// <summary>The archetype of a type. Never reads the table at runtime except through here.</summary>
	const EnemyArchetype& GetArchetype(EnemyType type);

	/// <summary>
	/// Console tuning (`enemy_tune`): sets one field of a type, for the enemies spawned after it. Fields: hp, wander_speed,
	/// chase_speed, turn_rate, trigger (also sets the lose radius to 1.2 x), attack_range (also the exit range to 1.2 x),
	/// damage, avoid. False for an unknown field or a value below 0.
	/// </summary>
	bool TuneArchetype(EnemyType type, const std::string& field, float value);

	/// <summary>The field names TuneArchetype takes, for the usage text.</summary>
	const char* GetTuneFields();

	/// <summary>Parses "drifter", "stalker" or "brute" (case-insensitive). Returns false for anything else.</summary>
	bool ParseEnemyType(const std::string& name, EnemyType& type);
}
