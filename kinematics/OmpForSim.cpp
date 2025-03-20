#include "kinematics.h"
#include <cassert>
#include <raylib.h>

namespace kinematics
{
OmpForSim::OmpForSim(const float width, const float height, const size_t numBodies)
	: StructOfVectorSim(width, height, numBodies) {};

OmpForSim::OmpForSim(const float width, const float height, const Simulation &toCopy)
	: StructOfVectorSim(width, height, toCopy) {};

void OmpForSim::UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                             float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed)
{
	const auto numBodies = GetNumBodies();
#pragma omp parallel for
	for (auto i = 0zu; i < numBodies; i++)
	{
		// Update position based on speed
		bodiesX[i] += bodiesHorizontalSpeed[i] * deltaTime;
		bodiesY[i] += bodiesVerticalSpeed[i] * deltaTime;

		// Bounce horizontally
		if (bodiesX[i] - BODY_RADIUS < 0 && bodiesHorizontalSpeed[i] < 0)
		{
			bodiesHorizontalSpeed[i] *= -1;
		}
		if (bodiesX[i] + BODY_RADIUS > _width && bodiesHorizontalSpeed[i] > 0)
		{
			bodiesHorizontalSpeed[i] *= -1;
		}

		// Bounce vertically
		if (bodiesY[i] - BODY_RADIUS < 0 && bodiesVerticalSpeed[i] < 0)
		{
			bodiesVerticalSpeed[i] *= -1;
		}
		if (bodiesY[i] + BODY_RADIUS > _height && bodiesVerticalSpeed[i] > 0)
		{
			bodiesVerticalSpeed[i] *= -1;
		}
	}
}
} // namespace kinematics
