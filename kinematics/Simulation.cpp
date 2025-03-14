#include "kinematics.h"
#include <cassert>
#include <raylib.h>

namespace kinematics
{
Simulation::Simulation(const float width, const float height) : _width(width), _height(height)
{
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

void Simulation::SetNumBodies([[maybe_unused]] const size_t totalNumBodies) {}

void Simulation::SetBounds(const float width, const float height)
{
	_width = width;
	_height = height;
}

Body Simulation::GenerateRandomBody()
{
	return Body{// Random starting position of a body that is in bounds
	            .x = static_cast<float>(GetRandomValue(BODY_RADIUS, static_cast<int>(_width - BODY_RADIUS))),
	            .y = static_cast<float>(GetRandomValue(BODY_RADIUS, static_cast<int>(_height - BODY_RADIUS))),

	            // Random starting speeds of body
	            .horizontalSpeed = static_cast<float>(GetRandomValue(-100, 100)) * SPEED_MODIFIER,
	            .verticalSpeed = static_cast<float>(GetRandomValue(-100, 100)) * SPEED_MODIFIER,

	            // Random (non-transparent) color of body
	            .color = {static_cast<unsigned char>(GetRandomValue(0, 255)),
	                      static_cast<unsigned char>(GetRandomValue(0, 255)),
	                      static_cast<unsigned char>(GetRandomValue(0, 255)), 255}};
}
} // namespace kinematics
