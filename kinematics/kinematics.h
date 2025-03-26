#pragma once
#include <array>
#include <raylib.h>
#include <vector>

#if __has_cpp_attribute(assume)
#define ASSUME(...) [[assume(__VA_ARGS__)]]
#else
#define ASSUME(...) __builtin_assume(__VA_ARGS__)
#endif

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
	Simulation(const float width, const float height);

	virtual ~Simulation();

	/// Progress the simulation by `deltaTime` seconds
	/// @param deltaTime Time in seconds to progress the simulation
	virtual void Update(const float deltaTime) = 0;

	/// Draw contents to the screen
	virtual void Draw() const = 0;

	/// Set the number of bodies in the simulation. Bodies will be created or removed to match the input value.
	/// @totalNumBodies The amount of bodies to have in the simulation
	virtual void SetNumBodies(const size_t totalNumBodies) = 0;

	/// @returns The number of bodies in the simulation
	virtual size_t GetNumBodies() const = 0;

	/// @returns A vector of copies of the contained bodies
	virtual std::vector<Body> GetBodies() const = 0;

	/// Set the bounds of the simulation
	void SetBounds(const float width, const float height);

  protected:
	Body GenerateRandomBody();

	// TODO: there's probably a better place for this
	virtual void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
	                          float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed);

  private:
	virtual void AddRandomBody() = 0;

  protected:
	float _width, _height;
	RenderTexture2D _bodyRender;
};

class VectorOfStructSim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	VectorOfStructSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	VectorOfStructSim(const float width, const float height, const Simulation &toCopy);

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

  private:
	void AddRandomBody() override;

  private:
	std::vector<Body> _bodies;
};

class StructOfVectorSim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	StructOfVectorSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	StructOfVectorSim(const float width, const float height, const Simulation &toCopy);

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

  private:
	void AddBody(const Body body); // TODO: should this be on base class?
	void AddRandomBody() override;

  protected:
	struct Bodies
	{
		std::vector<float> x, y; // center position
		std::vector<float> horizontalSpeed, verticalSpeed;
		std::vector<Color> color;
	};
	Bodies _bodies;
};

class OmpSimdSim : public StructOfVectorSim
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	OmpSimdSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	OmpSimdSim(const float width, const float height, const Simulation &toCopy);

	void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
	                  float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed) override;
};

class OmpForSim : public StructOfVectorSim
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	OmpForSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	OmpForSim(const float width, const float height, const Simulation &toCopy);

	void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
	                  float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed) override;
};

class StructOfPointerSim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	StructOfPointerSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	StructOfPointerSim(const float width, const float height, const Simulation &toCopy);
	~StructOfPointerSim();

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

  private:
	void AddBody(const Body body);
	void AddRandomBody() override;

  private:
	struct Bodies
	{
		float *x, *y; // center position
		float *horizontalSpeed, *verticalSpeed;
		Color *color;
	};
	Bodies _bodies;
	size_t _numBodies;
	size_t _maxBodies;
};

// TODO: Could refactor for less duplication with StructOfPointerSim
class StructOfAlignedSim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	StructOfAlignedSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	StructOfAlignedSim(const float width, const float height, const Simulation &toCopy);
	~StructOfAlignedSim();

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

	void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
	                  float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed) override;

  private:
	void AddBody(const Body body);
	void AddRandomBody() override;

  private:
	struct Bodies
	{
		float *x, *y; // center position
		float *horizontalSpeed, *verticalSpeed;
		Color *color;
	};
	Bodies _bodies;
	size_t _numBodies;
	size_t _maxBodies;
};

// TODO: Could refactor for less duplication with StructOfAlignedSim
class StructOfOversizedSim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	StructOfOversizedSim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	StructOfOversizedSim(const float width, const float height, const Simulation &toCopy);
	~StructOfOversizedSim();

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

	void UpdateHelper(const float deltaTime, float *__restrict__ bodiesX, float *__restrict__ bodiesY,
	                  float *__restrict__ bodiesHorizontalSpeed, float *__restrict__ bodiesVerticalSpeed) override;

  private:
	void AddBody(const Body body);
	void AddRandomBody() override;

  private:
	struct Bodies
	{
		float *x, *y; // center position
		float *horizontalSpeed, *verticalSpeed;
		Color *color;
	};
	Bodies _bodies;
	size_t _numBodies;
	size_t _maxBodies;
	size_t _updateBoundary;
};

template <size_t MAX_SIZE> class StructOfArraySim : public Simulation
{
  public:
	/// @param numBodies The number of bodies to initially add to the simulation
	StructOfArraySim(const float width, const float height, const size_t numBodies);

	/// @param toCopy Simulation containing the bodies to initially copy to this simulation. The originals will not be
	/// modified.
	StructOfArraySim(const float width, const float height, const Simulation &toCopy);

	void Update(const float deltaTime) override;
	void Draw() const override;
	void SetNumBodies(const size_t totalNumBodies) override;
	size_t GetNumBodies() const override;
	std::vector<Body> GetBodies() const override;

  private:
	void AddBody(const Body body);
	void AddRandomBody() override;

  private:
	struct Bodies
	{
		std::array<float, MAX_SIZE> x, y; // center position
		std::array<float, MAX_SIZE> horizontalSpeed, verticalSpeed;
		std::array<Color, MAX_SIZE> color;
	};
	Bodies _bodies;
	size_t _numBodies;
};
} // namespace kinematics
