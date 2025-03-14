#include <catch2/catch_all.hpp>
#include <raylib.h>
#include <string>

#include "../kinematics/kinematics.h"

TEST_CASE("Simulation")
{
	auto size = static_cast<size_t>(GENERATE(1'000, 10'000, 100'000, 1'000'000, 5'000'000));
	kinematics::Simulation simulation(800, 600, size);

	BENCHMARK("Update Simulation: " + std::to_string(size))
	{
		// `Update()` has obvious side-effects so it isn't ideal to reuse `simulation`, but may be accurate enough for
		// this benchmark
		return simulation.Update(1.f / 60.f);
	};
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
