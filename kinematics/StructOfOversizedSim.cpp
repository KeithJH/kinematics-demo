#include "kinematics.h"
#include <cassert>
#include <memory>
#include <raylib.h>
#include <vector>

namespace kinematics
{
constexpr size_t ALIGNMENT_SIZE = 64;
constexpr std::align_val_t ALIGNMENT = std::align_val_t(ALIGNMENT_SIZE);
constexpr size_t BODY_ALLOCATE_SIZE = 16;

size_t CalculateUpdateBoundary(const size_t size)
{
	const auto remainder = size % BODY_ALLOCATE_SIZE;
	return remainder ? size + BODY_ALLOCATE_SIZE - remainder : size;
}

StructOfOversizedSim::StructOfOversizedSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height), _numBodies(0), _maxBodies(0), _updateBoundary(0)
{
	// Calculate memory to allocate which helps allow vector "overflow" past the actual number of bodies
	_updateBoundary = CalculateUpdateBoundary(numBodies);

	// Allocate initial memory that can fit all the bodies
	_bodies.x = new (ALIGNMENT) float[_updateBoundary];
	_bodies.y = new (ALIGNMENT) float[_updateBoundary];
	_bodies.horizontalSpeed = new (ALIGNMENT) float[_updateBoundary];
	_bodies.verticalSpeed = new (ALIGNMENT) float[_updateBoundary];
	_bodies.color = new Color[_updateBoundary]; // TODO: could consider just using numBodies here
	_maxBodies = _updateBoundary;

	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);

	// Debug sanity checks
	assert(_numBodies == numBodies);
	assert(_maxBodies >= _numBodies);
	assert(_updateBoundary >= _numBodies);
	assert(_updateBoundary % BODY_ALLOCATE_SIZE == 0);
}
StructOfOversizedSim::StructOfOversizedSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height), _numBodies(0), _maxBodies(0), _updateBoundary(0)
{
	const auto totalNumBodies = toCopy.GetNumBodies();
	_updateBoundary = CalculateUpdateBoundary(totalNumBodies);

	_maxBodies = _updateBoundary;
	_bodies.x = new (ALIGNMENT) float[_updateBoundary];
	_bodies.y = new (ALIGNMENT) float[_updateBoundary];
	_bodies.horizontalSpeed = new (ALIGNMENT) float[_updateBoundary];
	_bodies.verticalSpeed = new (ALIGNMENT) float[_updateBoundary];

	_bodies.color = new Color[_updateBoundary];

	for (const auto &body : toCopy.GetBodies())
	{
		AddBody(body);
	}

	// Debug sanity checks
	assert(_numBodies == totalNumBodies);
	assert(_maxBodies >= _numBodies);
	assert(_updateBoundary >= _numBodies);
	assert(_updateBoundary % BODY_ALLOCATE_SIZE == 0);
}

StructOfOversizedSim::~StructOfOversizedSim()
{
	::operator delete[](_bodies.x, ALIGNMENT);
	::operator delete[](_bodies.y, ALIGNMENT);

	::operator delete[](_bodies.horizontalSpeed, ALIGNMENT);
	::operator delete[](_bodies.verticalSpeed, ALIGNMENT);

	delete[] _bodies.color;
}

std::vector<Body> StructOfOversizedSim::GetBodies() const
{
	std::vector<Body> copy;
	copy.reserve(GetNumBodies());

	const auto numBodies = GetNumBodies();
	for (size_t i = 0; i < numBodies; i++)
	{
		copy.emplace_back(_bodies.x[i], _bodies.y[i], _bodies.horizontalSpeed[i], _bodies.verticalSpeed[i],
		                  _bodies.color[i]);
	}

	return copy;
}

void StructOfOversizedSim::Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x, _bodies.y, _bodies.horizontalSpeed, _bodies.verticalSpeed);
}

void StructOfOversizedSim::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	const auto numBodies = GetNumBodies();
	for (size_t i = 0; i < numBodies; i++)
	{
		DrawTexture(_bodyRender.texture, static_cast<int>(_bodies.x[i] - BODY_RADIUS),
		            static_cast<int>(_bodies.y[i] - BODY_RADIUS), _bodies.color[i]);
	}
}

void StructOfOversizedSim::SetNumBodies(const size_t totalNumBodies)
{
	_updateBoundary = CalculateUpdateBoundary(totalNumBodies);
	if (totalNumBodies > _maxBodies)
	{
		// Lazy way to preserve the old bodies
		auto oldBodies = GetBodies();

		// Free previous memory
		::operator delete[](_bodies.x, ALIGNMENT);
		::operator delete[](_bodies.y, ALIGNMENT);

		::operator delete[](_bodies.horizontalSpeed, ALIGNMENT);
		::operator delete[](_bodies.verticalSpeed, ALIGNMENT);

		delete[] _bodies.color;

		// Allocate new memory that can fit all the bodies
		_bodies.x = new (ALIGNMENT) float[_updateBoundary];
		_bodies.y = new (ALIGNMENT) float[_updateBoundary];
		_bodies.horizontalSpeed = new (ALIGNMENT) float[_updateBoundary];
		_bodies.verticalSpeed = new (ALIGNMENT) float[_updateBoundary];

		_bodies.color = new Color[_updateBoundary];

		// Lazy way to add old bodies back
		_numBodies = 0;
		for (const auto &body : oldBodies)
		{
			AddBody(body);
		}
		_maxBodies = _updateBoundary;
	}

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
		// Does not shrink, so _maxBodies remains as-is
	}
}

size_t StructOfOversizedSim::GetNumBodies() const { return _numBodies; }

void StructOfOversizedSim::AddRandomBody() { AddBody(GenerateRandomBody()); }

void StructOfOversizedSim::AddBody(const Body body)
{
	_bodies.x[_numBodies] = body.x;
	_bodies.y[_numBodies] = body.y;

	_bodies.horizontalSpeed[_numBodies] = body.horizontalSpeed;
	_bodies.verticalSpeed[_numBodies] = body.verticalSpeed;

	_bodies.color[_numBodies] = body.color;
	_numBodies++;
}

void StructOfOversizedSim::UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                                        float *__restrict__ bodiesHorizontalSpeed,
                                        float *__restrict__ bodiesVerticalSpeed)
{
	bodiesX = std::assume_aligned<ALIGNMENT_SIZE>(bodiesX);
	bodiesY = std::assume_aligned<ALIGNMENT_SIZE>(bodiesY);
	bodiesHorizontalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesHorizontalSpeed);
	bodiesVerticalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesVerticalSpeed);

	// This **should** be enough to tell some compilers that it needn't worry as much about boundary conditions as the
	// work should be equally divisible by vector instructions.
	const auto numBodies = _updateBoundary;
	assert(numBodies % 16 == 0);
	ASSUME(numBodies % 16 == 0);

	for (size_t i = 0; i < numBodies; i++)
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
