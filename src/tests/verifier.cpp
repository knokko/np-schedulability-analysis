#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/solution.hpp"
#include "reconfiguration/verifier.hpp"

using namespace NP;

TEST_CASE("Verifier") {
	Global::State_space<dtime_t>::Workload jobs {
			// When job 0 or 1 goes first, everything is fine
			Job<dtime_t>{0, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 0, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 17), Interval<dtime_t>(8, 8), 50, 1, 1, 1},

			// When job 2 goes first, jobs 0 and 1 miss their deadlines
			Job<dtime_t>{2, Interval<dtime_t>(10, 17), Interval<dtime_t>(100, 100), 900, 2, 2, 2},
	};

	const auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());
	REQUIRE(!Reconfiguration::verify_solution(&problem, std::vector<Reconfiguration::Solution<dtime_t>*>()));

	std::vector<Reconfiguration::Solution<dtime_t>*> solution;
	solution.push_back(new Reconfiguration::Precedence_solution<dtime_t>(jobs[0].get_id(), jobs[2].get_id()));
	REQUIRE(Reconfiguration::verify_solution(&problem, solution));
}

#endif
