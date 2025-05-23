#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

#if __has_cpp_attribute(assume)
#define ASSUME(...) [[assume(__VA_ARGS__)]]
#else
#define ASSUME(...) __builtin_assume(__VA_ARGS__)
#endif

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
	constexpr size_t ALIGNMENT_SIZE = 64;
	constexpr std::align_val_t ALIGNMENT = std::align_val_t(ALIGNMENT_SIZE);
	constexpr size_t POINTS_MULTIPLE = 16;
	struct Points
	{
		float *position;
		float *speed;
	};

	Points points;

	const size_t remainder = numPoints % POINTS_MULTIPLE;
	const size_t numPointsOversized = remainder ? numPoints + POINTS_MULTIPLE - remainder : numPoints;
	points.position = new (ALIGNMENT) float[numPointsOversized];
	points.speed = new (ALIGNMENT) float[numPointsOversized];

	// Some compilers require the below (C++20 feature) to "see" the alignment for optimization
#if __cpp_lib_assume_aligned
	points.position = std::assume_aligned<ALIGNMENT_SIZE>(points.position);
	points.speed = std::assume_aligned<ALIGNMENT_SIZE>(points.speed);
#endif

	// Create pseudo-random points so the result doesn't get optimized to a constant
	for (size_t i = 0; i < numPoints; i++)
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
	for (size_t i = 0; i < numIterations; i++)
	{
		ASSUME(numPointsOversized % POINTS_MULTIPLE == 0);
		for (size_t point = 0; point < numPointsOversized; point++)
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

	// Free memory for points
	::operator delete[](points.position, ALIGNMENT);
	::operator delete[](points.speed, ALIGNMENT);

	return 0;
}
