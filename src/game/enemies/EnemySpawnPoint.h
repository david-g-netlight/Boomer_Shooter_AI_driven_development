#pragma once
#include <eng.h>

namespace enemies
{
	/// <summary>
	/// Marks an actor as a place where the WaveDirector may spawn an enemy (plan §6.2). Only the actor's world
	/// position is used. Saved in the level file, so spawn points can be moved in the editor.
	/// </summary>
	class EnemySpawnPoint : public eng::Component
	{
	public:
		bool			IsSpawnEnabled() const	{ return m_enabled; }
		void			SetSpawnEnabled(bool enabled) { m_enabled = enabled; }

		void			Serialize(eng::PropertyBag& out) const override;
		void			Deserialize(const eng::PropertyBag& in) override;
		const char*		TypeName() const override { return "EnemySpawnPoint"; }

	private:
		bool			m_enabled	= true;
	};
}
