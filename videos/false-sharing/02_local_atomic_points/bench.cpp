#include <array>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t MAX_THREADS = 8;
constexpr size_t NUM_POINTS_PER_THREAD = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _02_local_atomic_points {
struct Point
{
	std::atomic<float> position = 1.23f;
	std::atomic<float> velocity = 4.56f;

	void Update()
	{
		// position += velocity * DELTA_TIME;
		position.fetch_add(velocity * DELTA_TIME, std::memory_order_relaxed);
	}
};

static void Update(benchmark::State& state)
{
	std::array<Point, NUM_POINTS_PER_THREAD> points {};

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		for (auto &point : points)
		{
			point.Update();
		}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_02_local_atomic_points::Update);
BENCHMARK(_02_local_atomic_points::Update)->UseRealTime()->ThreadRange(2, MAX_THREADS);
