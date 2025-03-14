#include <catch2/catch_all.hpp>
#include <memory>
#include <raylib.h>
#include <string>

#include "../kinematics/kinematics.h"

TEST_CASE("Update")
{
	auto size = static_cast<size_t>(GENERATE(1'000, 10'000, 100'000, 1'000'000, 5'000'000));

	auto vectorOfStructSim = std::make_unique<kinematics::VectorOfStructSim>(800, 600, size);
	auto structOfVectorSim = std::make_unique<kinematics::StructOfVectorSim>(800, 600, *vectorOfStructSim.get());
	auto structOfArraySim =
		std::make_unique<kinematics::StructOfArraySim<5'000'000>>(800, 600, *vectorOfStructSim.get());

	// `Update()` has obvious side-effects so it isn't ideal to reuse simulations, but may be accurate enough for
	// this benchmark

	constexpr float TIME_CONSTANT = 1.f / 60.f;
	BENCHMARK("Update VectorOfStructSim: " + std::to_string(size)) { return vectorOfStructSim->Update(TIME_CONSTANT); };
	BENCHMARK("Update StructOfVectorSim: " + std::to_string(size)) { return structOfVectorSim->Update(TIME_CONSTANT); };
	BENCHMARK("Update StructOfArraySim: " + std::to_string(size)) { return structOfArraySim->Update(TIME_CONSTANT); };
}

// TODO: This should be split into proper tests, but gives good confidence for benchmark as-is
TEST_CASE("Copy")
{
	auto size = static_cast<size_t>(GENERATE(1'000, 10'000));

	auto original = std::make_unique<kinematics::VectorOfStructSim>(800, 600, size);

	auto vectorOfStructSim = std::make_unique<kinematics::VectorOfStructSim>(800, 600, *original.get());
	auto structOfVectorSim = std::make_unique<kinematics::StructOfVectorSim>(800, 600, *vectorOfStructSim.get());
	auto structOfArraySim =
		std::make_unique<kinematics::StructOfArraySim<5'000'000>>(800, 600, *structOfVectorSim.get());

	// Basic sanity test that all the sizes are set correctly
	REQUIRE(original->GetNumBodies() == size);
	REQUIRE(vectorOfStructSim->GetNumBodies() == size);
	REQUIRE(structOfVectorSim->GetNumBodies() == size);
	REQUIRE(structOfArraySim->GetNumBodies() == size);

	auto originalBodies = vectorOfStructSim->GetBodies();
	auto vectorOfStructBodies = vectorOfStructSim->GetBodies();
	auto structOfVectorBodies = structOfVectorSim->GetBodies();
	auto structOfArrayBodies = structOfArraySim->GetBodies();

	for (auto i = 0zu; i < size; i++)
	{
		REQUIRE(originalBodies[i].x == vectorOfStructBodies[i].x);
		REQUIRE(originalBodies[i].y == vectorOfStructBodies[i].y);
		REQUIRE(originalBodies[i].horizontalSpeed == vectorOfStructBodies[i].horizontalSpeed);
		REQUIRE(originalBodies[i].verticalSpeed == vectorOfStructBodies[i].verticalSpeed);
		REQUIRE(originalBodies[i].color.r == vectorOfStructBodies[i].color.r);
		REQUIRE(originalBodies[i].color.g == vectorOfStructBodies[i].color.g);
		REQUIRE(originalBodies[i].color.b == vectorOfStructBodies[i].color.b);
		REQUIRE(originalBodies[i].color.a == vectorOfStructBodies[i].color.a);

		REQUIRE(originalBodies[i].x == structOfVectorBodies[i].x);
		REQUIRE(originalBodies[i].y == structOfVectorBodies[i].y);
		REQUIRE(originalBodies[i].horizontalSpeed == structOfVectorBodies[i].horizontalSpeed);
		REQUIRE(originalBodies[i].verticalSpeed == structOfVectorBodies[i].verticalSpeed);
		REQUIRE(originalBodies[i].color.r == structOfVectorBodies[i].color.r);
		REQUIRE(originalBodies[i].color.g == structOfVectorBodies[i].color.g);
		REQUIRE(originalBodies[i].color.b == structOfVectorBodies[i].color.b);
		REQUIRE(originalBodies[i].color.a == structOfVectorBodies[i].color.a);

		REQUIRE(originalBodies[i].x == structOfArrayBodies[i].x);
		REQUIRE(originalBodies[i].y == structOfArrayBodies[i].y);
		REQUIRE(originalBodies[i].horizontalSpeed == structOfArrayBodies[i].horizontalSpeed);
		REQUIRE(originalBodies[i].verticalSpeed == structOfArrayBodies[i].verticalSpeed);
		REQUIRE(originalBodies[i].color.r == structOfArrayBodies[i].color.r);
		REQUIRE(originalBodies[i].color.g == structOfArrayBodies[i].color.g);
		REQUIRE(originalBodies[i].color.b == structOfArrayBodies[i].color.b);
		REQUIRE(originalBodies[i].color.a == structOfArrayBodies[i].color.a);
	}
}

/// Ensures environment is setup before running benchmark, such as by setting the RNG seed used by raylib
int main(int argc, char *argv[])
{
	Catch::Session session;
	session.configData();

	// Let Catch process the command line as normal
	int catchInitResult = session.applyCommandLine(argc, argv);
	if (catchInitResult != 0)
	{
		return catchInitResult;
	}

	// Custom initialization
	SetRandomSeed(session.config().rngSeed());

	// Let Catch run as usual and return the number of failed tests
	int catchRunResult = session.run();
	return catchRunResult;
}
