#include <benchmark/benchmark.h>
#include "Points.hpp"

namespace _03_soa {
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
		for (size_t point = 0; point < points._numPoints; point++)
		{
			points._position[point] += points._velocity[point] * DELTA_TIME;

			if ((points._position[point] < 0 && points._velocity[point] < 0) ||
				(points._position[point] > POSITION_LIMIT && points._velocity[point] > 0))
			{
				points._velocity[point] *= -1;
			}
		}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_03_soa::call)->Arg(2000);
BENCHMARK(_03_soa::manualInline)->Arg(2000);
