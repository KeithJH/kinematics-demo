#include <benchmark/benchmark.h>
#include "Point.hpp"

namespace _01_aos {
static void call(benchmark::State& state)
{
	const size_t numPoints = static_cast<size_t>(state.range(0));
	std::vector<Point> points = Point::RandomPointVector(numPoints);

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

BENCHMARK(_01_aos::call)->Arg(2000);
