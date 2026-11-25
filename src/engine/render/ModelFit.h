#pragma once
#include <cstdint>
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// The extent of a model that FitModel scales to its size.
	/// </summary>
	enum class FitAxis : uint8_t
	{
		Z,			// length along Z (a gun: the barrel points to -Z)
		Largest,	// the largest of the three extents ("fits in a diameter")
	};

	/// <summary>
	/// Matrix that recenters a model (its bounds' center goes to the origin) and scales it so that the chosen
	/// extent equals `size`. Apply it first: fit · pose, so the pose works in fitted meters. A zero extent
	/// keeps the scale at 1.
	/// </summary>
	Matrix FitModel(const BoundingBox& bounds, FitAxis axis, float size);

	/// <summary>
	/// The uniform scale FitModel uses: `size` over the chosen extent (1 for a zero extent). For parts placed in
	/// the fitted model's space.
	/// </summary>
	float FitScale(const BoundingBox& bounds, FitAxis axis, float size);
}
