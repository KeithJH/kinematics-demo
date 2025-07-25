#pragma once
#include <vector>

namespace _02_large_structure {
constexpr inline float DELTA_TIME = 1.f / 60.f;
constexpr inline float POSITION_LIMIT = 1000.f;
constexpr inline float VELOCITY_LIMIT = POSITION_LIMIT / 100;

struct Point
{
public:
	float _position;
	float _velocity;
	float otherValuesInProductionCode[16];

public:
	Point(const float position, const float velocity);
	void Update();

	static std::vector<Point> RandomPointVector(const size_t size);
};
}
