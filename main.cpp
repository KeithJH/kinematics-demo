#include <raylib.h>

namespace kinematics
{
constexpr int BODY_RADIUS = 20;
constexpr float SPEED_MODIFIER = 60.0;

/// Basic structure for describing a body of the simulation
struct Body
{
	float x, y;	// center position
	float horizontalSpeed, verticalSpeed;
	Color color;
};

/// Describes how the simulated "world" behaves. Currently this includes a singular `Body` that bounces around the screen.
class Simulation
{
  public:
	/// Create a simulation bounded with the given `width` and `height`
	Simulation(const int width, const int height) : _width(width), _height(height)
	{
		_body = Body{
			.x = 100, .y = 100, .horizontalSpeed = SPEED_MODIFIER, .verticalSpeed = SPEED_MODIFIER, .color = DARKBLUE};
	}

	/// Progress the simulation by `deltaTime` seconds
	void Update(const float deltaTime)
	{
		// Update position based on speed
		_body.x += _body.horizontalSpeed * deltaTime;
		_body.y += _body.verticalSpeed * deltaTime;

		// Bounce horizontally
		if (_body.x - BODY_RADIUS < 0 && _body.horizontalSpeed < 0)
		{
			_body.horizontalSpeed *= -1;
		}
		else if (_body.x + BODY_RADIUS > _width && _body.horizontalSpeed > 0)
		{
			_body.horizontalSpeed *= -1;
		}

		// Bounce vertically
		if (_body.y - BODY_RADIUS < 0 && _body.verticalSpeed < 0)
		{
			_body.verticalSpeed *= -1;
		}
		else if (_body.y + BODY_RADIUS > _height && _body.verticalSpeed > 0)
		{
			_body.verticalSpeed *= -1;
		}
	}

	/// Draw contents to the screen
	void Draw() const
	{
		// TODO: avoid recalculating a circle every time
		DrawCircle(_body.x, _body.y, BODY_RADIUS, _body.color);
	}

  private:
	int _width, _height;
	Body _body;
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
	kinematics::Simulation sim(800, 600);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Kinematics Demo");
	SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		sim.Update(GetFrameTime());
		DrawFrame(sim);
	}

	CloseWindow();
	return 0;
}
