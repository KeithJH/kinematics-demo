#include <benchmark/benchmark.h>
#include "Points.hpp"

namespace _05_aligned {
static void call(benchmark::State& state)
{
	const size_t numPoints = static_cast<size_t>(state.range(0));
	Points points(numPoints);

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		points.Update();
		benchmark::ClobberMemory();
	}
}

static void manualInline(benchmark::State& state)
{
	const size_t numPoints = static_cast<size_t>(state.range(0));
	Points points(numPoints);

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		for (size_t block = 0; block < points._numPointBlocks; block++) {
		for (size_t point = 0; point < Points::BLOCK_SIZE; point++)
		{
			points._pointBlocks[block].position[point] += points._pointBlocks[block].velocity[point] * DELTA_TIME;

			if ((points._pointBlocks[block].position[point] < 0 && points._pointBlocks[block].velocity[point] < 0) ||
				(points._pointBlocks[block].position[point] > POSITION_LIMIT && points._pointBlocks[block].velocity[point] > 0))
			{
				points._pointBlocks[block].velocity[point] *= -1;
			}
		}}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_05_aligned::call)->Arg(2000);
BENCHMARK(_05_aligned::manualInline)->Arg(2000);
