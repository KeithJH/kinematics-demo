#include "kinematics.h"
#include <cassert>
#include <raylib.h>
#include <vector>

namespace kinematics
{
VectorOfStructSim::VectorOfStructSim(const float width, const float height, const size_t numBodies)
	: Simulation(width, height)
{
	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);
}

VectorOfStructSim::VectorOfStructSim(const float width, const float height, const Simulation &toCopy)
	: Simulation(width, height)
{
	for (const auto &body : toCopy.GetBodies())
	{
		_bodies.push_back(body);
	}
}

std::vector<Body> VectorOfStructSim::GetBodies() const
{
	std::vector<Body> copy;
	copy.reserve(GetNumBodies());

	for (const auto &body : _bodies)
	{
		// TODO: might be good to add explicit constructor to help avoid ordering issues?
		copy.emplace_back(body.x, body.y, body.horizontalSpeed, body.verticalSpeed, body.color);
	}

	return copy;
}

void VectorOfStructSim::Update(const float deltaTime)
{
	for (auto &body : _bodies)
	{
		// Update position based on speed
		body.x += body.horizontalSpeed * deltaTime;
		body.y += body.verticalSpeed * deltaTime;

		// Bounce horizontally
		if (body.x - BODY_RADIUS < 0 && body.horizontalSpeed < 0)
		{
			body.horizontalSpeed *= -1;
		}
		else if (body.x + BODY_RADIUS > _width && body.horizontalSpeed > 0)
		{
			body.horizontalSpeed *= -1;
		}

		// Bounce vertically
		if (body.y - BODY_RADIUS < 0 && body.verticalSpeed < 0)
		{
			body.verticalSpeed *= -1;
		}
		else if (body.y + BODY_RADIUS > _height && body.verticalSpeed > 0)
		{
			body.verticalSpeed *= -1;
		}
	}
}

void VectorOfStructSim::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	for (const auto &body : _bodies)
	{
		DrawTexture(_bodyRender.texture, static_cast<int>(body.x - BODY_RADIUS), static_cast<int>(body.y - BODY_RADIUS),
		            body.color);
	}
}

void VectorOfStructSim::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > GetNumBodies())
	{
		_bodies.reserve(totalNumBodies);
		// TODO: maybe refactor this up to super class?
		for (auto i = GetNumBodies(); i < totalNumBodies; i++)
		{
			AddRandomBody();
		}
	}
	else
	{
		_bodies.resize(totalNumBodies);
	}
}

size_t VectorOfStructSim::GetNumBodies() const { return _bodies.size(); }

void VectorOfStructSim::AddRandomBody() { _bodies.push_back(GenerateRandomBody()); }
} // namespace kinematics
