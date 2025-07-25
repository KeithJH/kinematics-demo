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

static void manualInline(benchmark::State& state)
{
	const size_t numPoints = static_cast<size_t>(state.range(0));
	std::vector<Point> points = Point::RandomPointVector(numPoints);

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		for (Point &point : points)
		{
			point._position += point._velocity * DELTA_TIME;

			if ((point._position < 0 && point._velocity < 0) ||
				(point._position > POSITION_LIMIT && point._velocity > 0))
			{
				point._velocity *= -1;
			}
		}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_01_aos::call)->Arg(2000);
BENCHMARK(_01_aos::manualInline)->Arg(2000);
