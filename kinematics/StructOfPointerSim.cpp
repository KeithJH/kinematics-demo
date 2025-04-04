#include "kinematics.h"
#include <cassert>
#include <raylib.h>
#include <vector>

namespace kinematics
{
StructOfPointerSim::StructOfPointerSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height), _numBodies(0), _maxBodies(0)
{
	// Allocate initial memory that can fit all the bodies
	_bodies.x = new float[numBodies];
	_bodies.y = new float[numBodies];
	_bodies.horizontalSpeed = new float[numBodies];
	_bodies.verticalSpeed = new float[numBodies];
	_bodies.color = new Color[numBodies];
	_maxBodies = numBodies;

	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);
}
StructOfPointerSim::StructOfPointerSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height), _numBodies(0), _maxBodies(0)
{
	const auto totalNumBodies = toCopy.GetNumBodies();

	_maxBodies = totalNumBodies;
	_bodies.x = new float[totalNumBodies];
	_bodies.y = new float[totalNumBodies];
	_bodies.horizontalSpeed = new float[totalNumBodies];
	_bodies.verticalSpeed = new float[totalNumBodies];

	_bodies.color = new Color[totalNumBodies];

	for (const auto &body : toCopy.GetBodies())
	{
		AddBody(body);
	}

	assert(_numBodies == totalNumBodies);
}

StructOfPointerSim::~StructOfPointerSim()
{
	delete[] _bodies.x;
	delete[] _bodies.y;

	delete[] _bodies.horizontalSpeed;
	delete[] _bodies.verticalSpeed;

	delete[] _bodies.color;
}

std::vector<Body> StructOfPointerSim::GetBodies() const
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

void StructOfPointerSim::Update(const float deltaTime)
{
	// TODO: is there a better way to use `__restrict__`?
	UpdateHelper(deltaTime, _bodies.x, _bodies.y, _bodies.horizontalSpeed, _bodies.verticalSpeed);
}

void StructOfPointerSim::Draw() const
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

void StructOfPointerSim::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > _maxBodies)
	{
		// Lazy way to preserve the old bodies
		auto oldBodies = GetBodies();

		// Free previous memory
		delete[] _bodies.x;
		delete[] _bodies.y;

		delete[] _bodies.horizontalSpeed;
		delete[] _bodies.verticalSpeed;

		delete[] _bodies.color;

		// Allocate new memory that can fit all the bodies
		_bodies.x = new float[totalNumBodies];
		_bodies.y = new float[totalNumBodies];
		_bodies.horizontalSpeed = new float[totalNumBodies];
		_bodies.verticalSpeed = new float[totalNumBodies];

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

size_t StructOfPointerSim::GetNumBodies() const { return _numBodies; }

void StructOfPointerSim::AddRandomBody() { AddBody(GenerateRandomBody()); }

void StructOfPointerSim::AddBody(const Body body)
{
	_bodies.x[_numBodies] = body.x;
	_bodies.y[_numBodies] = body.y;

	_bodies.horizontalSpeed[_numBodies] = body.horizontalSpeed;
	_bodies.verticalSpeed[_numBodies] = body.verticalSpeed;

	_bodies.color[_numBodies] = body.color;
	_numBodies++;
}

} // namespace kinematics
