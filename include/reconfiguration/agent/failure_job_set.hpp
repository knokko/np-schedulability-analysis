#ifndef RECONFIGURATION_AGENT_FAILURE_JOB_SET_H
#define RECONFIGURATION_AGENT_FAILURE_JOB_SET_H

namespace NP::Reconfiguration {
	template <class Time> class Agent_failure_job_set_search : public Agent<Time> {
		Index_collection* job_set;

	public:
		static bool find_all_jobs_on_paths_to_deadline_misses(Scheduling_problem<Time> &problem, Index_collection* destination) {
			Analysis_options test_options;
			test_options.early_exit = false;
			test_options.use_supernodes = false;

			Agent_failure_job_set_search agent;
			agent.job_set = destination;
			return Global::State_space<Time>::explore(
					problem, test_options, &agent
			)->is_schedulable();
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			for (Job_index candidate = 0; candidate < 64 * failed_node.get_scheduled_jobs().get_vector_size(); candidate++) {
				if (failed_node.get_scheduled_jobs().contains(candidate)) job_set->insert(candidate);
			}
		}
	};
}

#endif
