#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"

using namespace NP;

TEST_CASE("First released job is not ready") {
	Global::State_space<dtime_t>::Workload jobs{
			// high-frequency task
			Job<dtime_t>{0, Interval<dtime_t>(10,  10), Interval<dtime_t>(1, 1), 30, 1, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(20, 20), Interval<dtime_t>(2, 2), 30, 2, 1, 1}
	};

	std::vector<Precedence_constraint<dtime_t>> precedence_constraints{
		Precedence_constraint<dtime_t>(jobs[1].get_id(), jobs[0].get_id(), Interval<dtime_t>(0, 0))
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, precedence_constraints);

	Analysis_options test_options;
	test_options.early_exit = false;
	test_options.use_supernodes = false;

	CHECK(Global::State_space<dtime_t>::explore(problem, test_options)->is_schedulable());
}
