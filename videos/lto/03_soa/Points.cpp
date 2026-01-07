#include <random>
#include <vector>
#include "Points.hpp"

namespace _03_soa {
Points::Points(const size_t numPoints)
{
	_numPoints = numPoints;
	_position.reserve(numPoints);
	_velocity.reserve(numPoints);

	std::mt19937 gen {0};
	std::uniform_real_distribution<float> randomPosition(0, POSITION_LIMIT);
	std::uniform_real_distribution<float> randomVelocity(-VELOCITY_LIMIT, VELOCITY_LIMIT);

	for (size_t i = 0; i < numPoints; i++)
	{
		_position.push_back(randomPosition(gen));
		_velocity.push_back(randomVelocity(gen));
	}
}

void Points::Update()
{
	for (size_t point = 0; point < _numPoints; point++)
	{
		_position[point] += _velocity[point] * DELTA_TIME;

		if ((_position[point] < 0 && _velocity[point] < 0) ||
		    (_position[point] > POSITION_LIMIT && _velocity[point] > 0))
		{
			_velocity[point] *= -1;
		}
	}
}
}
