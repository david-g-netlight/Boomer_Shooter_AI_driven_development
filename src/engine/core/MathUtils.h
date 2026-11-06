#pragma once
#include <raylib.h>
#include <raymath.h>

namespace eng
{
	/// <summary>
	/// Splits a world matrix built as scale -> rotation -> translation (GameActor::GetLocalMatrix order)
	/// back into its parts. Use this instead of raymath's MatrixDecompose: that one reads the matrix rows
	/// ({m0, m4, m8}...) and returns a wrong rotation and scale for non-uniformly scaled, rotated objects.
	/// Here the scale is the length of each basis column ({m0, m1, m2}...). Shear is ignored.
	/// </summary>
	inline void DecomposeTransform(const Matrix& m, Vector3* translation, Quaternion* rotation, Vector3* scale)
	{
		Vector3 x = { m.m0, m.m1, m.m2 };
		Vector3 y = { m.m4, m.m5, m.m6 };
		Vector3 z = { m.m8, m.m9, m.m10 };

		Vector3 s = { Vector3Length(x), Vector3Length(y), Vector3Length(z) };
		if (Vector3DotProduct(x, Vector3CrossProduct(y, z)) < 0.0f)
			s.x = -s.x;	// mirrored: fold the reflection into one axis

		if (translation)
			*translation = { m.m12, m.m13, m.m14 };
		if (scale)
			*scale = s;
		if (rotation)
		{
			x = Vector3Scale(x, s.x != 0.0f ? 1.0f / s.x : 0.0f);
			y = Vector3Scale(y, s.y != 0.0f ? 1.0f / s.y : 0.0f);
			z = Vector3Scale(z, s.z != 0.0f ? 1.0f / s.z : 0.0f);

			// Basis vectors as the columns of a pure rotation matrix (raylib lists fields row by row)
			const Matrix r = { x.x, y.x, z.x, 0.0f,
							   x.y, y.y, z.y, 0.0f,
							   x.z, y.z, z.z, 0.0f,
							   0.0f, 0.0f, 0.0f, 1.0f };
			*rotation = QuaternionNormalize(QuaternionFromMatrix(r));
		}
	}
}
