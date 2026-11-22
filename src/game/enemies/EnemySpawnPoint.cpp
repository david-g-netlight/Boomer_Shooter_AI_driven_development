#include "EnemySpawnPoint.h"

namespace enemies
{
	void EnemySpawnPoint::Serialize(eng::PropertyBag& out) const
	{
		out.SetBool("enabled", m_enabled);	// always written: the console can only set keys that exist
	}

	void EnemySpawnPoint::Deserialize(const eng::PropertyBag& in)
	{
		m_enabled = in.GetBool("enabled", true);
	}
}
