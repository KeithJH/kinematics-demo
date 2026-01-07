#pragma once
#include <vector>

namespace _05_aligned {
constexpr inline float DELTA_TIME = 1.f / 60.f;
constexpr inline float POSITION_LIMIT = 1000.f;
constexpr inline float VELOCITY_LIMIT = POSITION_LIMIT / 100;

class Points
{
  public:
	constexpr static size_t ALIGNMENT_SIZE = 64;
	constexpr static size_t BLOCK_SIZE = 16;

	struct alignas(ALIGNMENT_SIZE) PointBlock
	{
		float position[BLOCK_SIZE];
		float velocity[BLOCK_SIZE];
	};

	std::vector<PointBlock> _pointBlocks;
	size_t _numPoints, _numPointBlocks;

  public:
	Points(const size_t numPoints);

	void Update();
};
}
