#include <array>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t MAX_THREADS = 8;
constexpr size_t NUM_POINTS_PER_THREAD = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _07_multiple_arrays {
struct Point
{
	std::atomic<float> position = 1.23f;
	std::atomic<float> velocity = 4.56f;

	void Update()
	{
		position.fetch_add(velocity * DELTA_TIME, std::memory_order_relaxed);
	}
};

class alignas(64) Points
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

std::array<Points, MAX_THREADS> pointsArray {};

static void Update(benchmark::State& state)
{
	const size_t threadId = static_cast<size_t>(state.thread_index());
	Points &points = pointsArray[threadId];

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		points.Update();
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_07_multiple_arrays::Update)->UseRealTime()->Threads(MAX_THREADS);
