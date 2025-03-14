#include "kinematics.h"
#include <cassert>
#include <raylib.h>
#include <vector>

namespace kinematics
{
Simulation::Simulation(const float width, const float height, const size_t numBodies) : _width(width), _height(height)
{
	// Add an initial `numBodies` bodies to the simulation
	SetNumBodies(numBodies);

	// Not the most robust, but avoid segfault trying to access window related functionality where a window is not
	// present (such as via the benchmark).
	// TODO: Compose a draw stategy?
	if (IsWindowReady())
	{
		// Create texture for all bodies to reuse
		_bodyRender = LoadRenderTexture(BODY_RADIUS * 2, BODY_RADIUS * 2);
		BeginTextureMode(_bodyRender);
		ClearBackground(BLANK);
		DrawCircle(BODY_RADIUS, BODY_RADIUS, BODY_RADIUS, WHITE);
		EndTextureMode();
	}
}

Simulation::~Simulation()
{
	// Only unload texture if window is still ready to avoid potential segfault
	if (IsWindowReady())
		UnloadRenderTexture(_bodyRender);
}

void Simulation::Update(const float deltaTime)
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

void Simulation::Draw() const
{
	// `Draw()` should not be called when a window is not available
	assert(IsWindowReady());

	for (const auto &body : _bodies)
	{
		DrawTexture(_bodyRender.texture, static_cast<int>(body.x - BODY_RADIUS), static_cast<int>(body.y - BODY_RADIUS),
		            body.color);
	}
}

void Simulation::SetNumBodies(const size_t totalNumBodies)
{
	if (totalNumBodies > GetNumBodies())
	{
		_bodies.reserve(totalNumBodies);
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

void Simulation::SetBounds(const float width, const float height)
{
	_width = width;
	_height = height;
}

size_t Simulation::GetNumBodies() const { return _bodies.size(); }

void Simulation::AddRandomBody()
{
	Body body;

	// Random starting position of a body that is in bounds
	body.x = static_cast<float>(GetRandomValue(BODY_RADIUS, static_cast<int>(_width - BODY_RADIUS)));
	body.y = static_cast<float>(GetRandomValue(BODY_RADIUS, static_cast<int>(_height - BODY_RADIUS)));

	// Random starting speeds of body
	body.horizontalSpeed = static_cast<float>(GetRandomValue(-100, 100)) * SPEED_MODIFIER;
	body.verticalSpeed = static_cast<float>(GetRandomValue(-100, 100)) * SPEED_MODIFIER;

	// Random (non-transparent) color of body
	body.color = {static_cast<unsigned char>(GetRandomValue(0, 255)),
	              static_cast<unsigned char>(GetRandomValue(0, 255)),
	              static_cast<unsigned char>(GetRandomValue(0, 255)), 255};

	_bodies.push_back(body);
}
} // namespace kinematics
