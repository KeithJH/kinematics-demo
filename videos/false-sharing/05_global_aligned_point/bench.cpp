#include <array>
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

constexpr size_t MAX_THREADS = 8;
constexpr size_t NUM_POINTS_PER_THREAD = 8;
constexpr float DELTA_TIME = 1.f/60;

namespace _05_global_aligned_point {
struct alignas(64) Point
{
	std::atomic<float> position = 1.23f;
	std::atomic<float> velocity = 4.56f;

	void Update()
	{
		position.fetch_add(velocity * DELTA_TIME, std::memory_order_relaxed);
	}
};

struct Points
{
private:
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

BENCHMARK(_05_global_aligned_point::UpdateByStride)->UseRealTime()->Threads(MAX_THREADS);
BENCHMARK(_05_global_aligned_point::UpdateByChunk)->UseRealTime()->Threads(MAX_THREADS);
