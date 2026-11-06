#include "PhysicsDebugDrawer.h"
#include "core/Log.h"
#include <algorithm>
#include <raylib.h>

namespace eng
{
	namespace
	{
		Color ToColor(const btVector3& color)
		{
			auto channel = [](btScalar c) { return static_cast<unsigned char>(std::clamp(c, 0.0f, 1.0f) * 255.0f); };
			return { channel(color.x()), channel(color.y()), channel(color.z()), 255 };
		}
	}

	void PhysicsDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
	{
		DrawLine3D(bt::ToRaylib(from), bt::ToRaylib(to), ToColor(color));
	}

	void PhysicsDebugDrawer::drawContactPoint(const btVector3& point, const btVector3& normal, btScalar /*distance*/,
											  int /*lifeTime*/, const btVector3& /*color*/)
	{
		// Short line along the contact normal
		DrawLine3D(bt::ToRaylib(point), bt::ToRaylib(point + normal * 0.2f), YELLOW);
	}

	void PhysicsDebugDrawer::reportErrorWarning(const char* warning)
	{
		Log::Warning("Bullet: %s", warning);
	}

	void PhysicsDebugDrawer::draw3dText(const btVector3& /*location*/, const char* /*text*/)
	{
		// Not used: labels go through eng::DebugDraw
	}

	void PhysicsDebugDrawer::setDebugMode(int mode)	{ m_mode = mode; }
	int PhysicsDebugDrawer::getDebugMode() const	{ return m_mode; }
}
