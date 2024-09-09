#ifndef RECONFIGURATION_MANAGER_H
#define RECONFIGURATION_MANAGER_H

#include <unordered_set>

#include "global/space.hpp"

namespace NP {
	template<class Time>
	class Reconfiguration_manager {
	public:
		static int run_with_automatic_reconfiguration(Scheduling_problem<Time> problem) {
			Analysis_options test_options;
			test_options.timeout = 0;
			test_options.max_depth = 0;
			test_options.early_exit = true; // TODO Change to false when early_exit is supported
			test_options.be_naive = false;
			test_options.use_supernodes = false;

			auto history_agent = Reconfiguration_agent_job_sequence_history<Time>();
			auto base_analysis = Global::State_space<Time>::explore(
				problem, test_options, &history_agent
			);

			if (base_analysis->is_schedulable()) {
				std::cout << "No reconfiguration needed\n";
				return 0;
			}

			if (history_agent.failures.size() == 0) {
				std::cout << "Not schedulable, but no missed deadlines?\n";
				return -1;
			}

			std::pmr::unordered_set<unsigned long> problematic_jobs;
			for (auto failure : history_agent.failures) {
				std::cout << "Failure: ";
				for (auto job : failure.chosen_job_ids) {
					problematic_jobs.insert(job);
					std::cout << job << ", ";
				}
				std::cout << failure.missed_job_id << "\n";
			}

			auto adapted_problem = problem;
			for (auto &job : adapted_problem.jobs) {
				if (problematic_jobs.find(job.get_job_id()) != problematic_jobs.end()) {
					job.assume_pessimistic_arrival();
					job.assume_pessimistic_running_time();
					std::cout << "new arrival time of " << job.get_job_id() << " is " << job.arrival_window() << "\n";
				} else std::cout << "skipped job " << job.get_job_id() << "\n";
			}

			auto adapted_agent = Reconfiguration_agent_job_sequence_history<Time>();
			auto adapted_analysis = Global::State_space<Time>::explore(
				adapted_problem, test_options, &adapted_agent
			);

			std::cout << "adapted is schedulable? " << adapted_analysis->is_schedulable() << "\n";
			for (auto failure : adapted_agent.failures) {
				std::cout << "Failure: ";
				for (auto job : failure.chosen_job_ids) {
					std::cout << job << ", ";
				}
				std::cout << failure.missed_job_id << "\n";
			}

			return 0;
		}
	};
}

#endif
