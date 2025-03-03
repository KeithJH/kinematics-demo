#include <raylib.h>
#include <vector>

namespace kinematics
{
constexpr int BODY_RADIUS = 20;
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
		// Add `numBodies` bodies to the simulation
		_bodies.reserve(numBodies);
		for (auto i = 0; i < numBodies; i++)
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
		// TODO: avoid recalculating a circle every time
		for (auto &body : _bodies)
		{
			DrawCircle(body.x, body.y, BODY_RADIUS, body.color);
		}
	}

  private:
	int _width, _height;
	std::vector<Body> _bodies;
};
} // namespace kinematics

/// Top level draw function for the entire scene.
/// Includes: Background, FPS counter and `Simulation` contents.
void DrawFrame(kinematics::Simulation &sim)
{
	BeginDrawing();

	ClearBackground(BEIGE);
	DrawFPS(10, 10);

	sim.Draw();

	EndDrawing();
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Kinematics Demo");
	SetTargetFPS(60);

	kinematics::Simulation sim(800, 600, 10);
	while (!WindowShouldClose())
	{
		sim.Update(GetFrameTime());
		DrawFrame(sim);
	}

	CloseWindow();
	return 0;
}
