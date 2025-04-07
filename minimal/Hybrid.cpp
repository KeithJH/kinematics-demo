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
	constexpr size_t BLOCK_SIZE = 16;
	struct PointBlock
	{
		float position[BLOCK_SIZE];
		float speed[BLOCK_SIZE];
	};

	PointBlock *points;

	const size_t numPointBlocks = (numPoints + BLOCK_SIZE - 1) / BLOCK_SIZE;
	points = new (ALIGNMENT) PointBlock[numPointBlocks];

	// Some compilers require the below (C++20 feature) to "see" the alignment for optimization
#if __cpp_lib_assume_aligned
	points = std::assume_aligned<ALIGNMENT_SIZE>(points);
#endif

	// Create pseudo-random points so the result doesn't get optimized to a constant
	for (size_t i = 0; i < numPoints; i++)
	{
		const float position = static_cast<float>(rand() % 100);
		const float speed = static_cast<float>(rand() % 1000) / 100.f;

		points[i / BLOCK_SIZE].position[i % BLOCK_SIZE] = position;
		points[i / BLOCK_SIZE].speed[i % BLOCK_SIZE] = speed;
	}

	// Time the actual update loop
	std::printf("Starting %zu update loops of %zu points (%zu blocks)...", numIterations, numPoints, numPointBlocks);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (size_t i = 0; i < numIterations; i++)
	{
		for (size_t block = 0; block < numPointBlocks; block++)
		{
			for (size_t point = 0; point < BLOCK_SIZE; point++)
			{
				points[block].position[point] += points[block].speed[point] * DELTA_TIME;

				if ((points[block].position[point] < 0 && points[block].speed[point] < 0) ||
				    (points[block].position[point] > POSITION_LIMIT && points[block].speed[point] > 0))
				{
					points[block].speed[point] *= -1;
				}
			}
		}
	}
	const auto endTime = std::chrono::steady_clock::now();

	const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	// Free memory for points
	::operator delete[](points, ALIGNMENT);

	return 0;
}
