#include <chrono>
#include <raylib.h>
#include <vector>

namespace kinematics
{
constexpr int BODY_RADIUS = 10;
constexpr float SPEED_MODIFIER = 2.4f;

/// Basic structure for describing a body of the simulation
struct Body
{
	float x, y; // center position
	float horizontalSpeed, verticalSpeed;
	Color color;
};

/// Describes how the simulated "world" behaves. This includes multiple `Body` objects that bounce around the screen.
class Simulation
{
  public:
	/// Create a simulation bounded with the given `width` and `height`.
	/// Note: This uses random numbers and as such the random seed should be set prior to calling, either directly with
	/// `SetRandomSeed(...)` or indirectly with `InitWindow(...)`
	/// @param width Width of the environment to create
	/// @param height Height of the environment to create
	/// @param numBodies The number of bodies to initially add to the simulation
	Simulation(const float width, const float height, const size_t numBodies) : _width(width), _height(height)
	{
		// Add an initial `numBodies` bodies to the simulation
		SetNumBodies(numBodies);

		// Create texture for all bodies to reuse
		_bodyRender = LoadRenderTexture(BODY_RADIUS * 2, BODY_RADIUS * 2);
		BeginTextureMode(_bodyRender);
		ClearBackground(BLANK);
		DrawCircle(BODY_RADIUS, BODY_RADIUS, BODY_RADIUS, WHITE);
		EndTextureMode();
	}

	~Simulation() { UnloadRenderTexture(_bodyRender); }

	/// Progress the simulation by `deltaTime` seconds
	/// @param deltaTime Time in seconds to progress the simulation
	void Update(const float deltaTime)
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

	/// Draw contents to the screen
	void Draw() const
	{
		for (const auto &body : _bodies)
		{
			DrawTexture(_bodyRender.texture, static_cast<int>(body.x - BODY_RADIUS),
			            static_cast<int>(body.y - BODY_RADIUS), body.color);
		}
	}

	/// Set the number of bodies in the simulation. Bodies will be created or removed to match the input value.
	/// @totalNumBodies The amount of bodies to have in the simulation
	void SetNumBodies(const size_t totalNumBodies)
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

	/// Set the bounds of the simulation
	void SetBounds(const float width, const float height)
	{
		_width = width;
		_height = height;
	}

	/// @returns The number of bodies in the simulation
	size_t GetNumBodies() const { return _bodies.size(); }

  private:
	void AddRandomBody()
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

	float _width, _height;
	std::vector<Body> _bodies;

	RenderTexture2D _bodyRender;
};
} // namespace kinematics

class App
{
  public:
	App(const float width, const float height, const size_t initialNumBodies)
		: _simulation(width, height, initialNumBodies)
	{
	}

	void Update()
	{
		_frameTimeSeconds = GetFrameTime();
		HandleInput();

		if (IsWindowResized())
		{
			_simulation.SetBounds(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
		}

		if (_updateBodies)
		{
			auto startUpdateTime = std::chrono::steady_clock::now();
			_simulation.Update(_frameTimeSeconds);
			auto endUpdateTime = std::chrono::steady_clock::now();

			_updateMicroseconds =
				std::chrono::duration_cast<std::chrono::microseconds>(endUpdateTime - startUpdateTime).count();
		}
	}

	/// Top level draw function for the entire scene.
	/// Includes: Background, FPS counter, statistics and `Simulation` contents.
	void DrawFrame() const
	{
		BeginDrawing();
		ClearBackground(BEIGE);

		if (_renderBodies)
		{
			_simulation.Draw();
		}

		if (_renderStats)
		{
			constexpr int SECONDS_TO_MICROS = 1'000'000;

			DrawRectangle(10, 10, 400, 150, DARKGRAY);
			DrawText(TextFormat("Bodies:\t%d\nFrame  Time (us):\t%.0f\nUpdate Time (us):\t%ld\nRender "
			                    "Bodies:\t%d\nUpdate Bodies:\t%d",
			                    _simulation.GetNumBodies(), static_cast<double>(_frameTimeSeconds * SECONDS_TO_MICROS),
			                    _updateMicroseconds, _renderBodies, _updateBodies),
			         20, 40, 20, WHITE);
		}

		DrawFPS(15, 15);
		EndDrawing();
	}

  private:
	bool _renderBodies = true, _renderStats = false;
	bool _updateBodies = true;
	kinematics::Simulation _simulation;

	float _frameTimeSeconds;
	long _updateMicroseconds;

	/// Update the simulation according to user input.
	/// Includes: Toggle for rendering bodies, toggle for updating bodies, setting number of bodies
	void HandleInput()
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
			_simulation.SetNumBodies(1 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_TWO))
			_simulation.SetNumBodies(2 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_THREE))
			_simulation.SetNumBodies(3 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_FOUR))
			_simulation.SetNumBodies(4 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_FIVE))
			_simulation.SetNumBodies(5 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_SIX))
			_simulation.SetNumBodies(6 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_SEVEN))
			_simulation.SetNumBodies(7 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_EIGHT))
			_simulation.SetNumBodies(8 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_NINE))
			_simulation.SetNumBodies(9 * SMALL_COUNT);
		else if (IsKeyPressed(KEY_ZERO))
			_simulation.SetNumBodies(10 * SMALL_COUNT);

		// Medium
		if (IsKeyPressed(KEY_KP_1))
			_simulation.SetNumBodies(1 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_2))
			_simulation.SetNumBodies(2 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_3))
			_simulation.SetNumBodies(3 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_4))
			_simulation.SetNumBodies(4 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_5))
			_simulation.SetNumBodies(5 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_6))
			_simulation.SetNumBodies(6 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_7))
			_simulation.SetNumBodies(7 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_8))
			_simulation.SetNumBodies(8 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_9))
			_simulation.SetNumBodies(9 * MEDIUM_COUNT);
		else if (IsKeyPressed(KEY_KP_0))
			_simulation.SetNumBodies(10 * MEDIUM_COUNT);

		// Large
		if (IsKeyPressed(KEY_F1))
			_simulation.SetNumBodies(1 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F2))
			_simulation.SetNumBodies(2 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F3))
			_simulation.SetNumBodies(3 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F4))
			_simulation.SetNumBodies(4 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F5))
			_simulation.SetNumBodies(5 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F6))
			_simulation.SetNumBodies(6 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F7))
			_simulation.SetNumBodies(7 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F8))
			_simulation.SetNumBodies(8 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F9))
			_simulation.SetNumBodies(9 * LARGE_COUNT);
		else if (IsKeyPressed(KEY_F10))
			_simulation.SetNumBodies(10 * LARGE_COUNT);
	}
};

void Run()
{
	App app(800, 600, 1);
	while (!WindowShouldClose())
	{
		app.Update();
		app.DrawFrame();
	}
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(800, 600, "Kinematics Demo");

	Run();

	CloseWindow();
	return 0;
}
