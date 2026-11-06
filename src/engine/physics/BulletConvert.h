#pragma once

/**
* ENGINE-INTERNAL: includes Bullet. Only the .cpp files in src/engine/physics may include this header;
* include it first, before any other Bullet header.
* raylib and Bullet are both right-handed, Y-up and in meters, so no axis is swapped.
*/

#include <cfloat>	// Bullet's btScalar.h uses FLT_EPSILON without including it
#include <raylib.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btVector3.h>

namespace eng::bt
{
	inline btVector3 ToBt(const Vector3& v)
	{
		return { v.x, v.y, v.z };
	}

	inline Vector3 ToRaylib(const btVector3& v)
	{
		return { v.x(), v.y(), v.z() };
	}

	// Both store x, y, z, w
	inline btQuaternion ToBt(const Quaternion& q)
	{
		return { q.x, q.y, q.z, q.w };
	}

	inline Quaternion ToRaylibQuaternion(const btQuaternion& q)
	{
		return { q.x(), q.y(), q.z(), q.w() };
	}

	/// <summary>
	/// Rotation + translation only: Bullet bodies have no scale (it lives on the collision shape).
	/// </summary>
	inline btTransform ToBt(const ::Transform& t)
	{
		return btTransform(ToBt(t.rotation), ToBt(t.translation));
	}

	inline ::Transform ToRaylib(const btTransform& t, const Vector3& scale = { 1.0f, 1.0f, 1.0f })
	{
		return { ToRaylib(t.getOrigin()), ToRaylibQuaternion(t.getRotation()), scale };
	}

	/// <summary>
	/// Both are column-major in memory (OpenGL layout); raylib's struct lists the fields row by row.
	/// </summary>
	inline Matrix ToRaylibMatrix(const btTransform& t)
	{
		btScalar m[16];
		t.getOpenGLMatrix(m);
		return { m[0], m[4], m[8],  m[12],
				 m[1], m[5], m[9],  m[13],
				 m[2], m[6], m[10], m[14],
				 m[3], m[7], m[11], m[15] };
	}
}
