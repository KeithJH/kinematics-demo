#include <array>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t MAX_THREADS = 8;
constexpr size_t NUM_POINTS_PER_THREAD = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _06_thread_local_points {
struct Point
{
	std::atomic<float> position = 1.23f;
	std::atomic<float> velocity = 4.56f;

	void Update()
	{
		position.fetch_add(velocity * DELTA_TIME, std::memory_order_relaxed);
	}
};

class Points
{
	std::array<Point, NUM_POINTS_PER_THREAD> _values;

public:
	void Update()
	{
		for (size_t i = 0; i < _values.size(); i++)
		{
			_values[i].Update();
		}
	}
};

thread_local Points threadLocalPoints {};

static void Update(benchmark::State& state)
{
	benchmark::DoNotOptimize(threadLocalPoints);
	for (auto _ : state)
	{
		threadLocalPoints.Update();
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_06_thread_local_points::Update)->UseRealTime()->Threads(MAX_THREADS);
