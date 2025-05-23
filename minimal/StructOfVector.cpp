#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char **argv)
{
	constexpr float DELTA_TIME = 1.f / 60.f;
	constexpr float POSITION_LIMIT = 1000.f;

	// Rudimentary command line parsing
	size_t numPoints = 1'000'000;
	size_t numIterations = 10'000;

	switch (argc)
	{
	case 3: // Number of iterations specified
		numIterations = static_cast<size_t>(std::atoi(argv[2]));
		[[fallthrough]];

	case 2: // Number of points specified
		numPoints = static_cast<size_t>(std::atoi(argv[1]));
		break;

	case 1: // No arguments specified
		[[fallthrough]];
	default:
		break;
	}

	// Create pseudo-random points so the result doesn't get optimized to a constant
	struct Points
	{
		std::vector<float> position;
		std::vector<float> speed;
	};

	Points points;
	points.position.reserve(numPoints);
	points.speed.reserve(numPoints);

	for (size_t i = 0; i < numPoints; i++)
	{
		const float position = static_cast<float>(rand() % 100);
		const float speed = static_cast<float>(rand() % 1000) / 100.f;

		points.position.emplace_back(position);
		points.speed.emplace_back(speed);
	}

	// Time the actual update loop
	std::printf("Starting %zu update loops of %zu points...", numIterations, numPoints);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (size_t i = 0; i < numIterations; i++)
	{
		for (size_t point = 0; point < numPoints; point++)
		{
			points.position[point] += points.speed[point] * DELTA_TIME;

			if ((points.position[point] < 0 && points.speed[point] < 0) ||
			    (points.position[point] > POSITION_LIMIT && points.speed[point] > 0))
			{
				points.speed[point] *= -1;
			}
		}
	}
	const auto endTime = std::chrono::steady_clock::now();

	const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	return 0;
}
