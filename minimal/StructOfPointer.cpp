#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

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

	// Allocate memory for points
	struct Points
	{
		float *position;
		float *speed;
	};

	Points points;
	points.position = new float[numPoints];
	points.speed = new float[numPoints];

	// Create pseudo-random points so the result doesn't get optimized to a constant
	for (auto i = 0zu; i < numPoints; i++)
	{
		const float position = static_cast<float>(rand() % 100);
		const float speed = static_cast<float>(rand() % 1000) / 100.f;

		points.position[i] = position;
		points.speed[i] = speed;
	}

	// Time the actual update loop
	std::printf("Starting %zu update loops of %zu points...", numIterations, numPoints);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (auto i = 0zu; i < numIterations; i++)
	{
		for (auto point = 0zu; point < numPoints; point++)
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

	const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	// Free memory for points
	delete[] points.position;
	delete[] points.speed;

	return 0;
}
