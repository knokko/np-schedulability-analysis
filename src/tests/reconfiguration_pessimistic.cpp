#ifndef CONFIG_PARALLEL
#include "doctest.h"

#include "global/space.hpp"
#include "reconfiguration/agent/failure_job_set.hpp"
#include "reconfiguration/index_collection.hpp"
#include "reconfiguration/pessimistic.hpp"
#include "reconfiguration/solution.hpp"

using namespace NP;

TEST_CASE("Pessimistic reconfiguration strategy") {
	#include "jobs1a.hpp"
	auto problem = Scheduling_problem<dtime_t>(jobs1a, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Index_collection interesting_jobs;
	Reconfiguration::Agent_failure_job_set_search<dtime_t>::find_all_jobs_on_paths_to_deadline_misses(problem, &interesting_jobs);

	auto result = Reconfiguration::Pessimistic_reconfigurator<dtime_t>(problem, &interesting_jobs).find_local_minimal_solution();

	CHECK(result.size() == 2);
	auto solution1 = dynamic_cast<Reconfiguration::PessimisticExecutionTimeSolution<dtime_t>*>(result[0]);
	CHECK(solution1);
	CHECK(solution1->job_id.task == 0);
	CHECK(solution1->job_id.job == 1);
	CHECK(solution1->bestCase == 1);
	CHECK(solution1->worstCase == 2);

	auto solution2 = dynamic_cast<Reconfiguration::PessimisticExecutionTimeSolution<dtime_t>*>(result[1]);
	CHECK(solution2);
	CHECK(solution2->job_id.task == 6);
	CHECK(solution2->job_id.job == 7);
	CHECK(solution2->bestCase == 7);
	CHECK(solution2->worstCase == 8);
}
#endif