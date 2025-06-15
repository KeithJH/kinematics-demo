#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
	constexpr float DELTA_TIME = 1.f / 60.f;
	constexpr float POSITION_LIMIT = 1000.f;

	if (argc < 3) return 1;
	size_t numPoints = static_cast<size_t>(std::atoi(argv[1]));
	size_t numIterations = static_cast<size_t>(std::atoi(argv[2]));

	constexpr size_t BLOCK_SIZE = 16;

	struct PointBlock
	{
		float position[BLOCK_SIZE];
		float velocity[BLOCK_SIZE];
	};

	PointBlock *pointBlocks;

	const size_t numPointBlocks = (numPoints + BLOCK_SIZE - 1) / BLOCK_SIZE;
	pointBlocks = new PointBlock[numPointBlocks];

	for (size_t i = 0; i < numPoints; i++)
	{
		pointBlocks[i / BLOCK_SIZE].position[i % BLOCK_SIZE] = static_cast<float>(rand() % 100);
		pointBlocks[i / BLOCK_SIZE].velocity[i % BLOCK_SIZE] = static_cast<float>(rand() % 1000) / 100.f;
	}

	std::printf("Starting %zu update loops of %zu points (%zu blocks of %zu)...", numIterations, numPoints, numPointBlocks, BLOCK_SIZE);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (size_t iteration = 0; iteration < numIterations; iteration++)
	{
		asm("");

		for (size_t block = 0; block < numPointBlocks; block++)
		{
			for (size_t point = 0; point < BLOCK_SIZE; point++)
			{
				pointBlocks[block].position[point] += pointBlocks[block].velocity[point] * DELTA_TIME;

				if ((pointBlocks[block].position[point] < 0 && pointBlocks[block].velocity[point] < 0) ||
				    (pointBlocks[block].position[point] > POSITION_LIMIT && pointBlocks[block].velocity[point] > 0))
				{
					pointBlocks[block].velocity[point] *= -1;
				}
			}
		}
	}
	const auto endTime = std::chrono::steady_clock::now();

	const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	delete[] pointBlocks;

	return 0;
}
