#include "kinematics.h"
#include <cassert>
#include <raylib.h>

namespace kinematics
{
template <size_t size>
StructOfArraySim<size>::StructOfArraySim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height), _numBodies(0)
{
	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);
}

template <size_t size>
StructOfArraySim<size>::StructOfArraySim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height), _numBodies(0)
{
	[[maybe_unused]] const auto totalNumBodies = toCopy.GetNumBodies();
	assert(size >= totalNumBodies);

	for (const auto &body : toCopy.GetBodies())
	{
		AddBody(body);
	}

	assert(_numBodies == totalNumBodies);
}

template <size_t size> std::vector<Body> StructOfArraySim<size>::GetBodies() const
{
	std::vector<Body> copy;
	copy.reserve(GetNumBodies());

	const auto numBodies = GetNumBodies();
	for (auto i = 0zu; i < numBodies; i++)
	{
		copy.emplace_back(_bodies.x[i], _bodies.y[i], _bodies.horizontalSpeed[i], _bodies.verticalSpeed[i],
		                  _bodies.color[i]);
	}

	return copy;
}

template <size_t size> void StructOfArraySim<size>::Update(const float deltaTime)
{
	// NOTE: Even without explicit `__restrict__` there was already decent alias detection
	UpdateHelper(deltaTime, _bodies.x.data(), _bodies.y.data(), _bodies.horizontalSpeed.data(), _bodies.verticalSpeed.data());
}

template <size_t size> void StructOfArraySim<size>::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	const auto numBodies = GetNumBodies();
	for (auto i = 0zu; i < numBodies; i++)
	{
		DrawTexture(_bodyRender.texture, static_cast<int>(_bodies.x[i] - BODY_RADIUS),
		            static_cast<int>(_bodies.y[i] - BODY_RADIUS), _bodies.color[i]);
	}
}

template <size_t size> void StructOfArraySim<size>::SetNumBodies(const size_t totalNumBodies)
{
	assert(totalNumBodies <= size);

	if (totalNumBodies > GetNumBodies())
	{
		for (auto i = GetNumBodies(); i < totalNumBodies; i++)
		{
			AddRandomBody();
		}
	}
	else
	{
		_numBodies = totalNumBodies;
	}
}

template <size_t size> size_t StructOfArraySim<size>::GetNumBodies() const { return _numBodies; }

template <size_t size> void StructOfArraySim<size>::AddRandomBody() { AddBody(GenerateRandomBody()); }

template <size_t size> void StructOfArraySim<size>::AddBody(const Body body)
{
	_bodies.x[_numBodies] = body.x;
	_bodies.y[_numBodies] = body.y;

	_bodies.horizontalSpeed[_numBodies] = body.horizontalSpeed;
	_bodies.verticalSpeed[_numBodies] = body.verticalSpeed;

	_bodies.color[_numBodies] = body.color;

	_numBodies++;
}

// Explicitly instantiate specializations so they can be used from the shared library
template class StructOfArraySim<1'000'000>;
template class StructOfArraySim<5'000'000>;
} // namespace kinematics
