#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../kinematics/kinematics.h"

TEST_CASE("Simulation")
{
	// TODO: check whether this gets the random seed set correctly
	kinematics::Simulation simulation(800, 600, 5'000'000);

	BENCHMARK("Update Simulation (5M)")
	{
		// `Update()` has obvious side-effects so it isn't ideal to reuse `simulation`, but may be accurate enough for
		// this benchmark
		return simulation.Update(1.f / 60.f);
	};
}
