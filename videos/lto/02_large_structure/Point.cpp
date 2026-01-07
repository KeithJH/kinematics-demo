#include <random>
#include <vector>
#include "Point.hpp"

namespace _02_large_structure {
Point::Point(const float position, const float velocity) : _position(position), _velocity(velocity) {}

std::vector<Point> Point::RandomPointVector(const size_t size)
{
	std::vector<Point> points;
	points.reserve(size);

	std::mt19937 gen {0};
	std::uniform_real_distribution<float> randomPosition(0, POSITION_LIMIT);
	std::uniform_real_distribution<float> randomVelocity(-VELOCITY_LIMIT, VELOCITY_LIMIT);

	for (size_t i = 0; i < size; i++)
	{
		float pos = randomPosition(gen);
		float vel = randomVelocity(gen);

		points.emplace_back(pos, vel);
	}

	return points;
}

void Point::Update()
{
	_position += _velocity * DELTA_TIME;

	if ((_position < 0 && _velocity < 0) ||
	    (_position > POSITION_LIMIT && _velocity > 0))
	{
		_velocity *= -1;
	}
}
}
