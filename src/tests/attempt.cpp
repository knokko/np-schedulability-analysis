#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/attempt.hpp"

using namespace NP;

TEST_CASE("Precedence_attempt") {
	Global::State_space<dtime_t>::Workload jobs{
			Job<dtime_t>{0, Interval<dtime_t>(0,  0), Interval<dtime_t>(1, 2), 10, 10, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 2), 20, 20, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 2), 30, 30, 2, 2}
	};

	const Interval<dtime_t> no_suspension(0, 0);

	std::vector<Precedence_constraint<dtime_t>> constraints{
			Precedence_constraint<dtime_t>{jobs[0].get_id(), jobs[2].get_id(), no_suspension}
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, constraints);

	Reconfiguration::Precedence_attempt<dtime_t> attempt(1, std::vector<Job_index> { 0, 2 });

	attempt.attempt(problem);

	REQUIRE(problem.prec.size() == 3);
	for (int index = 0; index < 3; index++) CHECK(problem.prec[index].get_suspension() == no_suspension);
	CHECK(problem.prec[0].get_fromID() == jobs[0].get_id());
	CHECK(problem.prec[0].get_toID() == jobs[2].get_id());

	CHECK(problem.prec[1].get_fromID() == jobs[1].get_id());
	CHECK(problem.prec[1].get_toID() == jobs[0].get_id());

	CHECK(problem.prec[2].get_fromID() == jobs[1].get_id());
	CHECK(problem.prec[2].get_toID() == jobs[2].get_id());

	attempt.undo(problem);

	REQUIRE(problem.prec.size() == 1);
	CHECK(problem.prec[0].get_suspension() == no_suspension);
	CHECK(problem.prec[0].get_fromID() == jobs[0].get_id());
	CHECK(problem.prec[0].get_toID() == jobs[2].get_id());
}
#endif
