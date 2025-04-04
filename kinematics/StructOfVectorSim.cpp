#include "kinematics.h"
#include <cassert>
#include <raylib.h>
#include <vector>

namespace kinematics
{
StructOfVectorSim::StructOfVectorSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height)
{
	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);
}
StructOfVectorSim::StructOfVectorSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height)
{
	const auto totalNumBodies = toCopy.GetNumBodies();
	_bodies.x.reserve(totalNumBodies);
	_bodies.y.reserve(totalNumBodies);
	_bodies.horizontalSpeed.reserve(totalNumBodies);
	_bodies.verticalSpeed.reserve(totalNumBodies);
	_bodies.color.reserve(totalNumBodies);

	for (const auto &body : toCopy.GetBodies())
	{
		AddBody(body);
	}
}

std::vector<Body> StructOfVectorSim::GetBodies() const
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

void StructOfVectorSim::Update(const float deltaTime)
{
	UpdateHelper(deltaTime, _bodies.x.data(), _bodies.y.data(), _bodies.horizontalSpeed.data(), _bodies.verticalSpeed.data());
}

void StructOfVectorSim::Draw() const
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

void StructOfVectorSim::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > GetNumBodies())
	{
		_bodies.x.reserve(totalNumBodies);
		_bodies.y.reserve(totalNumBodies);
		_bodies.horizontalSpeed.reserve(totalNumBodies);
		_bodies.verticalSpeed.reserve(totalNumBodies);
		_bodies.color.reserve(totalNumBodies);

		for (auto i = GetNumBodies(); i < totalNumBodies; i++)
		{
			AddRandomBody();
		}
	}
	else
	{
		_bodies.x.resize(totalNumBodies);
		_bodies.y.resize(totalNumBodies);
		_bodies.horizontalSpeed.resize(totalNumBodies);
		_bodies.verticalSpeed.resize(totalNumBodies);
		_bodies.color.resize(totalNumBodies);
	}
}

size_t StructOfVectorSim::GetNumBodies() const { return _bodies.x.size(); }

void StructOfVectorSim::AddRandomBody() { AddBody(GenerateRandomBody()); }

void StructOfVectorSim::AddBody(const Body body)
{
	_bodies.x.push_back(body.x);
	_bodies.y.push_back(body.y);

	_bodies.horizontalSpeed.push_back(body.horizontalSpeed);
	_bodies.verticalSpeed.push_back(body.verticalSpeed);

	_bodies.color.push_back(body.color);
}

} // namespace kinematics
