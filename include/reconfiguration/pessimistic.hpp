#ifndef RECONFIGURATION_PESSIMISTIC_H
#define RECONFIGURATION_PESSIMISTIC_H

#include <unordered_set>
#include "solution.hpp"

namespace NP::Reconfiguration {
	template<class Time> class PessimisticReconfigurator {
		std::pmr::unordered_set<JobID> critical_job_ids;
		Scheduling_problem<Time> *original_problem;
		Scheduling_problem<Time> adapted_problem;
		Analysis_options *test_options{};

		void update_critical_jobs(std::vector<FailedSequence> &failures) {
			for (const auto &failure : failures) {
				for (auto job_id : failure.chosen_job_ids) critical_job_ids.insert(job_id);
			}
			for (auto &job : adapted_problem.jobs) {
				if (critical_job_ids.find(job.get_id()) != critical_job_ids.end()) {
					job.assume_pessimistic_arrival();
					job.assume_pessimistic_running_time();
				}
			}
		}

		bool attempt_adapted_problem() {
			Agent_job_sequence_history<Time> agent;
			auto result = Global::State_space<Time>::explore(
				adapted_problem, *test_options, &agent
			);
			if (result->is_schedulable()) return true;

			update_critical_jobs(agent.failures);
			return false;
		}

		bool adapt_until_schedulable() {
			while (true) {
				const auto old_job_count = critical_job_ids.size();
				if (attempt_adapted_problem()) return true;
				if (old_job_count == critical_job_ids.size()) {
					std::cout << "Pessimistic solution failed: problematic job IDs are ";
					for (const auto job : critical_job_ids) std::cout << job << ", ";
					std::cout << "\n";
					return false;
				}
			}
		}

	public:
		PessimisticReconfigurator(
			Scheduling_problem<Time> &problem,
			std::vector<FailedSequence> failures,
			Analysis_options *test_options
		) : original_problem(&problem), adapted_problem(problem), test_options(test_options) {
			update_critical_jobs(failures);
		}

		std::vector<Solution*> find_local_minimal_solution() {
			// Return an empty vector when pessimism can't solve the problem
			if (!adapt_until_schedulable()) return {};

			std::vector<Solution*> solution;
			for (int job_index = 0; job_index < adapted_problem.jobs.size(); job_index++) {
				auto original_job = original_problem->jobs[job_index];
				if (critical_job_ids.find(original_job.get_id()) != critical_job_ids.end()) {
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
			}

			return solution;
		}
	};
}

#endif
