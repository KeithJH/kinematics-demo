#include <chrono>
#include <memory>
#include <raylib.h>

#include "App.h"

App::App(const float width, const float height, const size_t initialNumBodies)
	: _simulation(std::make_unique<kinematics::VectorOfStructSim>(width, height, initialNumBodies))
{
	if (initialNumBodies >= 1'000'000)
	{
		_renderBodies = false;
		_renderStats = true;
	}
}

void App::Update()
{
	_frameTimeSeconds = GetFrameTime();
	HandleInput();

	if (IsWindowResized())
	{
		_simulation->SetBounds(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
	}

	if (_updateBodies)
	{
		auto startUpdateTime = std::chrono::steady_clock::now();
		_simulation->Update(_frameTimeSeconds);
		auto endUpdateTime = std::chrono::steady_clock::now();

		_updateMicroseconds =
			std::chrono::duration_cast<std::chrono::microseconds>(endUpdateTime - startUpdateTime).count();
	}
}

void App::DrawFrame() const
{
	BeginDrawing();
	ClearBackground(BEIGE);

	if (_renderBodies)
	{
		_simulation->Draw();
	}

	if (_renderStats)
	{
		constexpr int SECONDS_TO_MICROS = 1'000'000;

		DrawRectangle(10, 10, 400, 150, DARKGRAY);
		DrawText(TextFormat("Bodies:\t%d\nFrame  Time (us):\t%.0f\nUpdate Time (us):\t%ld\nRender "
		                    "Bodies:\t%d\nUpdate Bodies:\t%d",
		                    _simulation->GetNumBodies(), static_cast<double>(_frameTimeSeconds * SECONDS_TO_MICROS),
		                    _updateMicroseconds, _renderBodies, _updateBodies),
		         20, 40, 20, WHITE);
	}

	DrawFPS(15, 15);
	EndDrawing();
}

void App::HandleInput()
{
	if (IsKeyPressed(KEY_R))
		_renderBodies = !_renderBodies;
	if (IsKeyPressed(KEY_S))
		_renderStats = !_renderStats;
	if (IsKeyPressed(KEY_U))
		_updateBodies = !_updateBodies;

	constexpr size_t SMALL_COUNT = 1;
	constexpr size_t MEDIUM_COUNT = 100'000;
	constexpr size_t LARGE_COUNT = 1'000'000;

	// Small
	if (IsKeyPressed(KEY_ONE))
		_simulation->SetNumBodies(1 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_TWO))
		_simulation->SetNumBodies(2 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_THREE))
		_simulation->SetNumBodies(3 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_FOUR))
		_simulation->SetNumBodies(4 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_FIVE))
		_simulation->SetNumBodies(5 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_SIX))
		_simulation->SetNumBodies(6 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_SEVEN))
		_simulation->SetNumBodies(7 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_EIGHT))
		_simulation->SetNumBodies(8 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_NINE))
		_simulation->SetNumBodies(9 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_ZERO))
		_simulation->SetNumBodies(10 * SMALL_COUNT);

	// Medium
	if (IsKeyPressed(KEY_KP_1))
		_simulation->SetNumBodies(1 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_2))
		_simulation->SetNumBodies(2 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_3))
		_simulation->SetNumBodies(3 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_4))
		_simulation->SetNumBodies(4 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_5))
		_simulation->SetNumBodies(5 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_6))
		_simulation->SetNumBodies(6 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_7))
		_simulation->SetNumBodies(7 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_8))
		_simulation->SetNumBodies(8 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_9))
		_simulation->SetNumBodies(9 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_0))
		_simulation->SetNumBodies(10 * MEDIUM_COUNT);

	// Large
	if (IsKeyPressed(KEY_F1))
		_simulation->SetNumBodies(1 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F2))
		_simulation->SetNumBodies(2 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F3))
		_simulation->SetNumBodies(3 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F4))
		_simulation->SetNumBodies(4 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F5))
		_simulation->SetNumBodies(5 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F6))
		_simulation->SetNumBodies(6 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F7))
		_simulation->SetNumBodies(7 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F8))
		_simulation->SetNumBodies(8 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F9))
		_simulation->SetNumBodies(9 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F10))
		_simulation->SetNumBodies(10 * LARGE_COUNT);
}
