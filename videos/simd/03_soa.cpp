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

	struct Points
	{
		float *position;
		float *velocity;
	};

	Points points;
	points.position = new float[numPoints];
	points.velocity = new float[numPoints];

	for (size_t i = 0; i < numPoints; i++)
	{
		points.position[i] = static_cast<float>(rand() % 100);
		points.velocity[i] = static_cast<float>(rand() % 1000) / 100.f;
	}

	std::printf("Starting %zu update loops of %zu points...", numIterations, numPoints);
	std::fflush(stdout);
	const auto startTime = std::chrono::steady_clock::now();
	for (size_t iteration = 0; iteration < numIterations; iteration++)
	{
		asm(""); // Keep outer loop intact

		for (size_t point = 0; point < numPoints; point++)
		{
			points.position[point] += points.velocity[point] * DELTA_TIME;

			if ((points.position[point] < 0 && points.velocity[point] < 0) ||
			    (points.position[point] > POSITION_LIMIT && points.velocity[point] > 0))
			{
				points.velocity[point] *= -1;
			}
		}
	}
	const auto endTime = std::chrono::steady_clock::now();

	const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::printf(" ran for %ldms\n", milliseconds);

	delete[] points.position;
	delete[] points.velocity;

	return 0;
}
