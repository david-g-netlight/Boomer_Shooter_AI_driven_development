#pragma once
#include <cstddef>
#include <memory>
#include <unordered_set>
#include "level/Component.h"

namespace eng
{
	class Collider;
	class GameActor;

	/// <summary>
	/// Turns the Collider on the same actor into a trigger: objects pass through it, and both actors'
	/// components get OnTriggerEnter / OnTriggerExit exactly once per enter / exit (checked every fixed tick).
	/// Only layers in the Collider's mask are detected (Layer::Trigger default: Player | Prop).
	/// A body destroyed while inside is dropped silently (no Exit), like Unity.
	/// The trigger follows its actor, so it can move.
	/// </summary>
	class TriggerVolume : public Component
	{
	public:
		TriggerVolume();
		~TriggerVolume() override;

		size_t		GetOverlapCount() const;
		bool		Contains(const GameActor& actor) const;

		void		OnEnable() override;
		void		OnDisable() override;
		void			DrawInspector(InspectorLayout& ui) override;
		const char*	TypeName() const override { return "TriggerVolume"; }

	private:
		struct Impl;
		std::unique_ptr<Impl>			m_impl;
		Collider*						m_collider	= nullptr;
		std::unordered_set<Collider*>	m_inside;		// confirmed overlaps since the last tick

		friend class PhysicsWorld;
	};
}
