#ifndef CONFIG_PARALLEL
#include "doctest.h"

#include "global/space.hpp"
#include "reconfiguration/agent/failure_job_set.hpp"
#include "reconfiguration/agent.hpp"
#include "jobs1a.hpp"

using namespace NP;

TEST_CASE("Agent_failure_job_set_search") {
	#include "jobs1a.hpp"
	auto problem = Scheduling_problem<dtime_t>(jobs1a, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Index_collection interesting_jobs;
	Reconfiguration::Agent_failure_job_set_search<dtime_t>::find_all_jobs_on_paths_to_deadline_misses(problem, &interesting_jobs);
	CHECK(interesting_jobs.contains(0));
	for (Job_index nope = 1; nope <= 5; nope++) CHECK(!interesting_jobs.contains(nope));
	CHECK(interesting_jobs.contains(6));
	CHECK(!interesting_jobs.contains(7));
	CHECK(interesting_jobs.contains(8));
}
#endif