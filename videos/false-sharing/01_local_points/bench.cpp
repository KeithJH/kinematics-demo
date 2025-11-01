#include <array>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t NUM_POINTS = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _01_local_points {
struct Point
{
	float position = 1.23f;
	float velocity = 4.56f;

	void Update() { position += velocity * DELTA_TIME; }
};

static void Update(benchmark::State& state)
{
	std::array<Point, NUM_POINTS> points {};

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		for (Point &point : points)
		{
			point.Update();
		}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_01_local_points::Update);
