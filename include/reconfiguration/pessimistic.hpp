#ifndef RECONFIGURATION_PESSIMISTIC_H
#define RECONFIGURATION_PESSIMISTIC_H

#include <unordered_set>
#include "solution.hpp"
#include "agent/failure_job_set.hpp"

namespace NP::Reconfiguration {
	template<class Time> class Pessimistic_reconfigurator {
		Index_collection* interesting_jobs;
		Scheduling_problem<Time> *original_problem;
		Scheduling_problem<Time> adapted_problem;

		bool attempt_adapted_problem() {
			Analysis_options test_options;
			test_options.early_exit = false;
			test_options.use_supernodes = false;

			auto space = Global::State_space<Time>::explore(adapted_problem, test_options, nullptr);
			bool result = space->is_schedulable();
			delete space;
			return result;
		}

	public:
		Pessimistic_reconfigurator(
				Scheduling_problem<Time> &problem, Index_collection *interesting_jobs
		) : original_problem(&problem), adapted_problem(problem), interesting_jobs(interesting_jobs) { }

		std::vector<Solution<Time>*> find_local_minimal_solution() {

			// Assume pessimistic arrival times and running times for all jobs
			for (const auto job_index : *interesting_jobs) {
				adapted_problem.jobs[job_index].assume_pessimistic_arrival();
				adapted_problem.jobs[job_index].assume_pessimistic_running_time();
			}

			// Return an empty vector when even that doesn't solve the problem
			if (!attempt_adapted_problem()) return {};

			std::vector<Solution<Time>*> solution;
			for (const auto job_index : *interesting_jobs) {
				auto original_job = original_problem->jobs[job_index];
				auto adapted_job = original_job;

				auto test_job = original_job;
				test_job.assume_pessimistic_arrival();
				adapted_problem.jobs[job_index] = test_job;
				if (!attempt_adapted_problem()) {
					auto pets = new PessimisticExecutionTimeSolution<Time>();
					pets->job_id = original_job.get_id();
					pets->bestCase = original_job.get_cost().min();
					pets->worstCase = original_job.get_cost().max();

					solution.push_back(pets);
					adapted_job.assume_pessimistic_running_time();
				}

				test_job = adapted_job;
				test_job.assume_pessimistic_running_time();
				adapted_problem.jobs[job_index] = test_job;
				if (!attempt_adapted_problem()) {
					auto pats = new PessimisticArrivalTimeSolution<Time>();
					pats->job_id = original_job.get_id();
					pats->earliest = original_job.earliest_arrival();
					pats->latest = original_job.latest_arrival();

					solution.push_back(pats);
					adapted_job.assume_pessimistic_arrival();
				}

				adapted_problem.jobs[job_index] = adapted_job;
			}

			return solution;
		}
	};
}

#endif
