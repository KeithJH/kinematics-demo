#include <raylib.h>
#include <vector>

namespace kinematics
{
constexpr int BODY_RADIUS = 10;
constexpr float SPEED_MODIFIER = 2.4;

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
	Simulation(const int width, const int height, const size_t numBodies) : _width(width), _height(height)
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
		for (auto &body : _bodies)
		{
			DrawTexture(_bodyRender.texture, body.x - BODY_RADIUS, body.y - BODY_RADIUS, body.color);
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

	/// @returns The number of bodies in the simulation
	size_t GetNumBodies() const { return _bodies.size(); }

  private:
	void AddRandomBody()
	{
		Body body;

		// Random starting position of a body that is in bounds
		body.x = GetRandomValue(BODY_RADIUS, _width - BODY_RADIUS);
		body.y = GetRandomValue(BODY_RADIUS, _height - BODY_RADIUS);

		// Random starting speeds of body
		body.horizontalSpeed = GetRandomValue(-100, 100) * SPEED_MODIFIER;
		body.verticalSpeed = GetRandomValue(-100, 100) * SPEED_MODIFIER;

		// Random (non-transparent) color of body
		body.color = {static_cast<unsigned char>(GetRandomValue(0, 255)),
		              static_cast<unsigned char>(GetRandomValue(0, 255)),
		              static_cast<unsigned char>(GetRandomValue(0, 255)), 255};

		_bodies.push_back(body);
	}

	int _width, _height;
	std::vector<Body> _bodies;

	RenderTexture2D _bodyRender;
};
} // namespace kinematics

/// Update the simulation according to user input. Currently used to set the number of bodies.
void HandleInput(kinematics::Simulation &sim)
{
	constexpr size_t SMALL_COUNT = 1;
	constexpr size_t MEDIUM_COUNT = 1'000;
	constexpr size_t LARGE_COUNT = 100'000;

	// Small
	if (IsKeyPressed(KEY_ONE)) sim.SetNumBodies(1 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_TWO)) sim.SetNumBodies(2 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_THREE)) sim.SetNumBodies(3 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_FOUR)) sim.SetNumBodies(4 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_FIVE)) sim.SetNumBodies(5 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_SIX)) sim.SetNumBodies(6 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_SEVEN)) sim.SetNumBodies(7 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_EIGHT)) sim.SetNumBodies(8 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_NINE)) sim.SetNumBodies(9 * SMALL_COUNT);
	else if (IsKeyPressed(KEY_ZERO)) sim.SetNumBodies(10 * SMALL_COUNT);

	// Medium
	if (IsKeyPressed(KEY_KP_1)) sim.SetNumBodies(1 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_2)) sim.SetNumBodies(2 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_3)) sim.SetNumBodies(3 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_4)) sim.SetNumBodies(4 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_5)) sim.SetNumBodies(5 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_6)) sim.SetNumBodies(6 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_7)) sim.SetNumBodies(7 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_8)) sim.SetNumBodies(8 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_9)) sim.SetNumBodies(9 * MEDIUM_COUNT);
	else if (IsKeyPressed(KEY_KP_0)) sim.SetNumBodies(10 * MEDIUM_COUNT);

	// Large
	if (IsKeyPressed(KEY_F1)) sim.SetNumBodies(1 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F2)) sim.SetNumBodies(2 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F3)) sim.SetNumBodies(3 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F4)) sim.SetNumBodies(4 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F5)) sim.SetNumBodies(5 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F6)) sim.SetNumBodies(6 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F7)) sim.SetNumBodies(7 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F8)) sim.SetNumBodies(8 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F9)) sim.SetNumBodies(9 * LARGE_COUNT);
	else if (IsKeyPressed(KEY_F10)) sim.SetNumBodies(10 * LARGE_COUNT);
}

/// Top level draw function for the entire scene.
/// Includes: Background, FPS counter and `Simulation` contents.
void DrawFrame(kinematics::Simulation &sim)
{
	BeginDrawing();

	ClearBackground(BEIGE);
	sim.Draw();
	DrawFPS(10, 10);

	EndDrawing();
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Kinematics Demo");
	SetTargetFPS(60);

	kinematics::Simulation sim(800, 600, 1);
	while (!WindowShouldClose())
	{
		HandleInput(sim);
		sim.Update(GetFrameTime());
		DrawFrame(sim);
	}

	CloseWindow();
	return 0;
}
