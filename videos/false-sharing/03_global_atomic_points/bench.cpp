#include <array>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t MAX_THREADS = 8;
constexpr size_t NUM_POINTS_PER_THREAD = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _03_global_atomic_points {
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
	std::array<Point, NUM_POINTS_PER_THREAD * MAX_THREADS> _values;

public:
	void UpdateByStride(const size_t threadId)
	{
		for (size_t i = threadId; i < _values.size(); i += MAX_THREADS)
		{
			_values[i].Update();
		}
	}

	void UpdateByChunk(const size_t threadId)
	{
		const size_t chunkStart = threadId * NUM_POINTS_PER_THREAD;
		for (size_t i = 0; i < NUM_POINTS_PER_THREAD; i++)
		{
			_values[i + chunkStart].Update();
		}
	}
};

Points globalPoints {};

static void UpdateByStride(benchmark::State& state)
{
	const size_t threadId = static_cast<size_t>(state.thread_index());

	benchmark::DoNotOptimize(globalPoints);
	for (auto _ : state)
	{
		globalPoints.UpdateByStride(threadId);
		benchmark::ClobberMemory();
	}
}

static void UpdateByChunk(benchmark::State& state)
{
	const size_t threadId = static_cast<size_t>(state.thread_index());

	benchmark::DoNotOptimize(globalPoints);
	for (auto _ : state)
	{
		globalPoints.UpdateByChunk(threadId);
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_03_global_atomic_points::UpdateByStride)->UseRealTime()->ThreadRange(1, MAX_THREADS);
BENCHMARK(_03_global_atomic_points::UpdateByChunk)->UseRealTime()->ThreadRange(1, MAX_THREADS);
