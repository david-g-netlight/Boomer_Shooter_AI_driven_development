#pragma once

/**
* ENGINE-INTERNAL (includes Bullet).
*/

#include "BulletConvert.h"
#include <LinearMath/btIDebugDraw.h>

namespace eng
{
	/// <summary>
	/// Bullet's debug drawing (collision shapes, contacts) through raylib. Call debugDrawWorld() between
	/// BeginMode3D / EndMode3D.
	/// </summary>
	class PhysicsDebugDrawer : public btIDebugDraw
	{
	public:
		void	drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
		void	drawContactPoint(const btVector3& point, const btVector3& normal, btScalar distance,
								 int lifeTime, const btVector3& color) override;
		void	reportErrorWarning(const char* warning) override;
		void	draw3dText(const btVector3& location, const char* text) override;
		void	setDebugMode(int mode) override;
		int		getDebugMode() const override;

	private:
		int		m_mode = DBG_DrawWireframe | DBG_DrawContactPoints;
	};
}
