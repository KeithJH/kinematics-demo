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
	Simulation(const float width, const float height, const size_t numBodies);

	~Simulation();

	/// Progress the simulation by `deltaTime` seconds
	/// @param deltaTime Time in seconds to progress the simulation
	void Update(const float deltaTime);

	/// Draw contents to the screen
	void Draw() const;

	/// Set the number of bodies in the simulation. Bodies will be created or removed to match the input value.
	/// @totalNumBodies The amount of bodies to have in the simulation
	void SetNumBodies(const size_t totalNumBodies);

	/// Set the bounds of the simulation
	void SetBounds(const float width, const float height);

	/// @returns The number of bodies in the simulation
	size_t GetNumBodies() const;

  private:
	void AddRandomBody();

  private:
	float _width, _height;
	std::vector<Body> _bodies;

	RenderTexture2D _bodyRender;
};
} // namespace kinematics
