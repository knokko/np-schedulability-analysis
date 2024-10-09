#include "doctest.h"

#include "global/space.hpp"
#include "reconfiguration/agent/failure_job_set.hpp"
#include "reconfiguration/agent.hpp"

using namespace NP;

TEST_CASE("Agent_failure_job_set_search") {
	Global::State_space<dtime_t>::Workload jobs{
			// high-frequency task
			Job<dtime_t>{1, Interval<dtime_t>(0,  0), Interval<dtime_t>(1, 2), 10, 10, 0, 0},
			Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 2), 20, 20, 1, 1},
			Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 2), 30, 30, 2, 2},
			Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(1, 2), 40, 40, 3, 3},
			Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(1, 2), 50, 50, 4, 4},
			Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(1, 2), 60, 60, 5, 5},

			// middle task
			Job<dtime_t>{7, Interval<dtime_t>(0,  0), Interval<dtime_t>(7, 8), 30, 30, 6, 6},
			Job<dtime_t>{8, Interval<dtime_t>(30, 30), Interval<dtime_t>(7, 8), 60, 60, 7, 7},

			// the long task
			Job<dtime_t>{9, Interval<dtime_t>(0,  0), Interval<dtime_t>(3, 13), 60, 60, 8, 8}
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Index_collection interesting_jobs;
	Reconfiguration::Agent_failure_job_set_search<dtime_t>::find_all_jobs_on_paths_to_deadline_misses(problem, &interesting_jobs);
	CHECK(interesting_jobs.contains(0));
	for (Job_index nope = 1; nope <= 5; nope++) CHECK(!interesting_jobs.contains(nope));
	CHECK(interesting_jobs.contains(6));
	CHECK(!interesting_jobs.contains(7));
	CHECK(interesting_jobs.contains(8));
}
