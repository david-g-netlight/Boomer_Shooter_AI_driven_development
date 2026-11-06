#pragma once
#include <cstdint>

namespace eng
{
	/// <summary>
	/// Collision groups (bit flags). Bullet reports a pair only when each side's group is in the other's
	/// mask, so DefaultMask() is kept symmetric. Raycasts take a mask of the layers they may hit
	/// (the scaling gun uses Ground | Prop, so it ignores the player and triggers).
	/// </summary>
	namespace Layer
	{
		enum : uint32_t
		{
			None	= 0,
			Ground	= 1u << 0,	// static level geometry
			Prop	= 1u << 1,	// crates, planks: scalable / dynamic
			Player	= 1u << 2,	// the character capsule
			Trigger	= 1u << 3,	// win / lose / next-level volumes
			Beam	= 1u << 4,	// reserved for beam / projectile queries

			All		= Ground | Prop | Player | Trigger | Beam,
		};
	}

	/// <summary>
	/// The layers an object on `layer` collides with by default.
	/// </summary>
	constexpr uint32_t DefaultCollisionMask(uint32_t layer)
	{
		switch (layer)
		{
			case Layer::Ground:		return Layer::Prop | Layer::Player | Layer::Beam;
			case Layer::Prop:		return Layer::Ground | Layer::Prop | Layer::Player | Layer::Trigger | Layer::Beam;
			case Layer::Player:		return Layer::Ground | Layer::Prop | Layer::Trigger;
			case Layer::Trigger:	return Layer::Prop | Layer::Player;
			case Layer::Beam:		return Layer::Ground | Layer::Prop;
			default:				return Layer::All;
		}
	}

	constexpr const char* LayerName(uint32_t layer)
	{
		switch (layer)
		{
			case Layer::Ground:		return "Ground";
			case Layer::Prop:		return "Prop";
			case Layer::Player:		return "Player";
			case Layer::Trigger:	return "Trigger";
			case Layer::Beam:		return "Beam";
			default:				return "Mixed";
		}
	}

	/// <summary>
	/// Compile-time check that the defaults are symmetric (a sees b exactly when b sees a).
	/// </summary>
	constexpr bool AreDefaultMasksSymmetric()
	{
		constexpr uint32_t layers[] = { Layer::Ground, Layer::Prop, Layer::Player, Layer::Trigger, Layer::Beam };
		for (uint32_t a : layers)
			for (uint32_t b : layers)
				if (((DefaultCollisionMask(a) & b) != 0) != ((DefaultCollisionMask(b) & a) != 0))
					return false;
		return true;
	}

	static_assert(AreDefaultMasksSymmetric(), "DefaultCollisionMask must be symmetric");
}
