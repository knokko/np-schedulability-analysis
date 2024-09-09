#ifndef RECONFIGURATION_PESSIMISTIC_H
#define RECONFIGURATION_PESSIMISTIC_H

#include <unordered_set>

namespace NP::Reconfiguration {
	template<class Time> class PessimisticReconfigurator {
		std::pmr::unordered_set<unsigned long> critical_job_ids;
		Scheduling_problem<Time> *original_problem;
		Scheduling_problem<Time> adapted_problem;
		Analysis_options *test_options{};

		void update_critical_jobs(std::vector<FailedSequence> &failures) {
			for (const auto &failure : failures) {
				for (auto job_id : failure.chosen_job_ids) critical_job_ids.insert(job_id);
			}
			for (auto &job : adapted_problem.jobs) {
				if (critical_job_ids.find(job.get_job_id()) != critical_job_ids.end()) {
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
				if (old_job_count == critical_job_ids.size()) return false;
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

		void find_local_minimal_solution() {
			if (!adapt_until_schedulable()) {
				std::cout << "Hopeless cause\n";
				return;
			}

			std::unordered_set<unsigned long> truly_critical_job_ids;
			for (int job_index = 0; job_index < adapted_problem.jobs.size(); job_index++) {
				auto original_job = original_problem->jobs[job_index];
				auto adapted_job = adapted_problem.jobs[job_index];
				if (critical_job_ids.find(adapted_job.get_job_id()) != critical_job_ids.end()) {
					adapted_problem.jobs[job_index] = original_job;
					if (!attempt_adapted_problem()) {
						truly_critical_job_ids.insert(adapted_job.get_job_id());
						adapted_problem.jobs[job_index] = adapted_job;
					}
				}
			}

			std::cout << "critical jobs: ";
			for (const auto job : critical_job_ids) std::cout << job << ", ";
			std::cout << "\ntruly critical jobs: ";
			for (const auto job : truly_critical_job_ids) std::cout << job << ", ";
			std::cout << "\n";
		}
	};
}

#endif
