#include "ModelFit.h"
#include <algorithm>
#include <raymath.h>

namespace eng
{
	Matrix FitModel(const BoundingBox& bounds, FitAxis axis, float size)
	{
		const Vector3 extent = Vector3Subtract(bounds.max, bounds.min);
		const Vector3 center = Vector3Scale(Vector3Add(bounds.min, bounds.max), 0.5f);

		float length = extent.z;
		if (axis == FitAxis::Largest)
			length = std::max({ extent.x, extent.y, extent.z });

		const float scale = length > 0.0f ? size / length : 1.0f;

		// Translate first (multiplied on the right), then scale
		return MatrixMultiply(MatrixScale(scale, scale, scale), MatrixTranslate(-center.x, -center.y, -center.z));
	}
}
