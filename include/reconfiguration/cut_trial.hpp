#ifndef CUT_TRIAL_H
#define CUT_TRIAL_H

#include "agent.hpp"
#include "attachment.hpp"
#include "graph_cutter.hpp"

namespace NP::Reconfiguration {
	template<class Time> class Agent_cut_check : public Agent<Time> {
		Rating_graph_cut cut;
		bool has_expected_failure;
		bool has_unexpected_failure;
		bool has_correct_dead_end;

		bool has_taken_previous_cut_jobs(const Global::Schedule_node<Time> &node) {
			for (auto job_index : *cut.previous_jobs) {
				if (!node.get_scheduled_jobs().contains(job_index)) return false;
			}
			return true;
		}
	public:
		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			bool was_expected = has_taken_previous_cut_jobs(failed_node) && failed_node.get_scheduled_jobs().contains(cut.forbidden_job);

			std::cout << "missed deadline of job " << late_job.get_job_index() << " expected? " << was_expected << std::endl;
			if (was_expected) has_expected_failure = true;
			else has_unexpected_failure = true;
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			bool took_previous_jobs = has_taken_previous_cut_jobs(dead_node);
			bool took_forbidden_job = dead_node.get_scheduled_jobs().contains(cut.forbidden_job);

			std::cout << "dead end after " << dead_node.get_scheduled_jobs().size() << " took previous? " << took_forbidden_job << " took forbidden? " << took_forbidden_job << std::endl;
			if (took_previous_jobs && !took_forbidden_job && dead_node.get_scheduled_jobs().size() == cut.previous_jobs->size()) {
				has_correct_dead_end = true;
				return;
			}

			if (took_previous_jobs && took_forbidden_job) has_expected_failure = true;
			else has_unexpected_failure = true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			return cut.previous_jobs->contains(next_job.get_job_index()) || cut.forbidden_job == next_job.get_job_index();
		}
	};
}

#endif
