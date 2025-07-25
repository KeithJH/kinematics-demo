#pragma once
#include <vector>

namespace _03_soa {
constexpr inline float DELTA_TIME = 1.f / 60.f;
constexpr inline float POSITION_LIMIT = 1000.f;
constexpr inline float VELOCITY_LIMIT = POSITION_LIMIT / 100;

struct Points
{
public:
	std::vector<float> _position;
	std::vector<float> _velocity;
	size_t _numPoints;

public:
	Points(const size_t numPoints);

	void Update();
};
}
