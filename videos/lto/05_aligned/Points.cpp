#include <random>
#include <vector>
#include "Points.hpp"

namespace _05_aligned {
Points::Points(const size_t numPoints)
{
	_numPoints = numPoints;
	_numPointBlocks = (numPoints + BLOCK_SIZE - 1) / BLOCK_SIZE;
	_pointBlocks.reserve(_numPointBlocks);

	std::mt19937 gen {0};
	std::uniform_real_distribution<float> randomPosition(0, POSITION_LIMIT);
	std::uniform_real_distribution<float> randomVelocity(-VELOCITY_LIMIT, VELOCITY_LIMIT);

	for (size_t i = 0; i < numPoints; i++)
	{
		_pointBlocks[i / BLOCK_SIZE].position[i % BLOCK_SIZE] = randomPosition(gen);
		_pointBlocks[i / BLOCK_SIZE].velocity[i % BLOCK_SIZE] = randomVelocity(gen);
	}
}

void Points::Update()
{
	for (size_t block = 0; block < _numPointBlocks; block++) {
	for (size_t point = 0; point < BLOCK_SIZE; point++)
	{
		_pointBlocks[block].position[point] += _pointBlocks[block].velocity[point] * DELTA_TIME;

		if ((_pointBlocks[block].position[point] < 0 && _pointBlocks[block].velocity[point] < 0) ||
		    (_pointBlocks[block].position[point] > POSITION_LIMIT && _pointBlocks[block].velocity[point] > 0))
		{
			_pointBlocks[block].velocity[point] *= -1;
		}
	}}
}
}
