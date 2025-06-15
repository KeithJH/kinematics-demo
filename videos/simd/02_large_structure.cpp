#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char **argv)
{
	constexpr float DELTA_TIME = 1.f / 60.f;
	constexpr float POSITION_LIMIT = 1000.f;

	if (argc < 3) return 1;
	size_t numPoints = static_cast<size_t>(std::atoi(argv[1]));
	size_t numIterations = static_cast<size_t>(std::atoi(argv[2]));

	struct Point
	{
		float position;
		float velocity;
		float otherValuesInProductionCode[16];
	};

	std::vector<Point> points;
	points.reserve(numPoints);

	for (size_t i = 0; i < numPoints; i++)
	{
		points.emplace_back(static_cast<float>(rand() % 100),
				    static_cast<float>(rand() % 1000) / 100.f);
	}

	std::printf("Starting %zu update loops of %zu points...", numIterations, numPoints);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (size_t iteration = 0; iteration < numIterations; iteration++)
	{
		asm("");

		for (Point &point : points)
		{
			point.position += point.velocity * DELTA_TIME;

			if ((point.position < 0 && point.velocity < 0) ||
			    (point.position > POSITION_LIMIT && point.velocity > 0))
			{
				point.velocity *= -1;
			}
		}
	}
	const auto endTime = std::chrono::steady_clock::now();

	const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	return 0;
}
