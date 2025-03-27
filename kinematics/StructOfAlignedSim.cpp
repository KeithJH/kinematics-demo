#include "kinematics.h"
#include <cassert>
#include <memory>
#include <raylib.h>
#include <vector>

namespace kinematics
{
constexpr size_t ALIGNMENT_SIZE = 64;
constexpr std::align_val_t ALIGNMENT = std::align_val_t(ALIGNMENT_SIZE);

StructOfAlignedSim::StructOfAlignedSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height), _numBodies(0), _maxBodies(0)
{
	// Allocate initial memory that can fit all the bodies
	_bodies.x = new (ALIGNMENT) float[numBodies];
	_bodies.y = new (ALIGNMENT) float[numBodies];
	_bodies.horizontalSpeed = new (ALIGNMENT) float[numBodies];
	_bodies.verticalSpeed = new (ALIGNMENT) float[numBodies];
	_bodies.color = new Color[numBodies];
	_maxBodies = numBodies;

	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);
}
StructOfAlignedSim::StructOfAlignedSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height), _numBodies(0), _maxBodies(0)
{
	const auto totalNumBodies = toCopy.GetNumBodies();

	_maxBodies = totalNumBodies;
	_bodies.x = new (ALIGNMENT) float[totalNumBodies];
	_bodies.y = new (ALIGNMENT) float[totalNumBodies];
	_bodies.horizontalSpeed = new (ALIGNMENT) float[totalNumBodies];
	_bodies.verticalSpeed = new (ALIGNMENT) float[totalNumBodies];

	_bodies.color = new Color[totalNumBodies];

	for (const auto &body : toCopy.GetBodies())
	{
		AddBody(body);
	}

	assert(_numBodies == totalNumBodies);
}

StructOfAlignedSim::~StructOfAlignedSim()
{
	::operator delete[](_bodies.x, ALIGNMENT);
	::operator delete[](_bodies.y, ALIGNMENT);

	::operator delete[](_bodies.horizontalSpeed, ALIGNMENT);
	::operator delete[](_bodies.verticalSpeed, ALIGNMENT);

	delete[] _bodies.color;
}

std::vector<Body> StructOfAlignedSim::GetBodies() const
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

void StructOfAlignedSim::Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x, _bodies.y, _bodies.horizontalSpeed, _bodies.verticalSpeed);
}

void StructOfAlignedSim::Draw() const
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

void StructOfAlignedSim::SetNumBodies(const size_t totalNumBodies)
{
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
		_bodies.x = new (ALIGNMENT) float[totalNumBodies];
		_bodies.y = new (ALIGNMENT) float[totalNumBodies];
		_bodies.horizontalSpeed = new (ALIGNMENT) float[totalNumBodies];
		_bodies.verticalSpeed = new (ALIGNMENT) float[totalNumBodies];

		_bodies.color = new Color[totalNumBodies];

		// Lazy way to add old bodies back
		_numBodies = 0;
		for (const auto &body : oldBodies)
		{
			AddBody(body);
		}
		_maxBodies = totalNumBodies;
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

size_t StructOfAlignedSim::GetNumBodies() const { return _numBodies; }

void StructOfAlignedSim::AddRandomBody() { AddBody(GenerateRandomBody()); }

void StructOfAlignedSim::AddBody(const Body body)
{
	_bodies.x[_numBodies] = body.x;
	_bodies.y[_numBodies] = body.y;

	_bodies.horizontalSpeed[_numBodies] = body.horizontalSpeed;
	_bodies.verticalSpeed[_numBodies] = body.verticalSpeed;

	_bodies.color[_numBodies] = body.color;
	_numBodies++;
}

void StructOfAlignedSim::UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
                                      float *__restrict__ bodiesHorizontalSpeed,
                                      float *__restrict__ bodiesVerticalSpeed)
{
	// TODO: is there a better way to declare/enforce alignment
	bodiesX = std::assume_aligned<ALIGNMENT_SIZE>(bodiesX);
	bodiesY = std::assume_aligned<ALIGNMENT_SIZE>(bodiesY);
	bodiesHorizontalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesHorizontalSpeed);
	bodiesVerticalSpeed = std::assume_aligned<ALIGNMENT_SIZE>(bodiesVerticalSpeed);

	const auto numBodies = GetNumBodies();
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
