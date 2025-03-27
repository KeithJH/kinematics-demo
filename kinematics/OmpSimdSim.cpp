#include "kinematics.h"
#include <cassert>
#include <raylib.h>

namespace kinematics
{
OmpSimdSim::OmpSimdSim(const float width, const float height, const size_t numBodies)
	: StructOfVectorSim(width, height, numBodies) {};

OmpSimdSim::OmpSimdSim(const float width, const float height, const Simulation &toCopy)
	: StructOfVectorSim(width, height, toCopy) {};

void OmpSimdSim::UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                              float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed)
{
	const auto numBodies = GetNumBodies();
#pragma omp simd
	for (auto i = 0zu; i < numBodies; i++)
	{
		// Update position based on speed
		bodiesX[i] += bodiesHorizontalSpeed[i] * deltaTime;
		bodiesY[i] += bodiesVerticalSpeed[i] * deltaTime;

		// Bounce horizontally
		if (BounceCheck(bodiesX[i], bodiesHorizontalSpeed[i], _width))
		{
			bodiesHorizontalSpeed[i] *= -1;
		}

		// Bounce vertically
		if (BounceCheck(bodiesY[i], bodiesVerticalSpeed[i], _height))
		{
			bodiesVerticalSpeed[i] *= -1;
		}
	}
}
} // namespace kinematics
