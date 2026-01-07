#include <benchmark/benchmark.h>
#include <random>
#include <vector>

namespace _01_aos {
static void singleSource(benchmark::State& state)
{
	constexpr float DELTA_TIME = 1.f / 60.f;
	constexpr float POSITION_LIMIT = 1000.f;
	constexpr float VELOCITY_LIMIT = POSITION_LIMIT / 100;

	const size_t numPoints = static_cast<size_t>(state.range(0));

	struct Point
	{
		float position;
		float velocity;
	};

	std::vector<Point> points;
	points.reserve(numPoints);

	std::mt19937 gen {0};
	std::uniform_real_distribution<float> randomPosition(0, POSITION_LIMIT);
	std::uniform_real_distribution<float> randomVelocity(-VELOCITY_LIMIT, VELOCITY_LIMIT);

	for (size_t i = 0; i < numPoints; i++)
	{
		points.emplace_back(randomVelocity(gen), randomPosition(gen));
	}

	benchmark::DoNotOptimize(points);
	for (auto _ : state)
	{
		for (Point &point : points)
		{
			point.position += point.velocity * DELTA_TIME;

			if ((point.position < 0 && point.velocity < 0) ||
				(point.position > POSITION_LIMIT && point.velocity > 0))
			{
				point.velocity *= -1;
			}
		}
		benchmark::ClobberMemory();
	}
}
}

BENCHMARK(_01_aos::singleSource)->Arg(2000);
