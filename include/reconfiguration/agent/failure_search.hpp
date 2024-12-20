#ifndef RECONFIGURATION_AGENT_FAILURE_SEARCH_H
#define RECONFIGURATION_AGENT_FAILURE_SEARCH_H

#include <vector>
#include "../job_sequence.hpp"
#include "problem.hpp"
#include "job_sequence_history.hpp"

namespace NP::Reconfiguration {
	template <class Time> class Agent_failure_search : public Agent<Time> {
		int paths_without_deadline_misses = 0;
		std::vector<Job_sequence> failures;
		bool did_update_failures = true;

		void add_failure(const Job_sequence &failure) {
			for (int index = 0; index < failures.size(); index++) {
				if (failure.is_prefix_of(failures[index], true)) {
					failures[index] = failures[failures.size() - 1];
					failures.pop_back();
					did_update_failures = true;
				} else if (failures[index].is_prefix_of(failure, false)) return;
			}
			failures.push_back(failure);
			did_update_failures = true;
		}
	public:
		static std::vector<Job_sequence> find_all_failures(Scheduling_problem<Time> &problem, Analysis_options &test_options) {
			Agent_failure_search agent;
			while (agent.did_update_failures) {
				delete Global::State_space<Time>::explore(
						problem, test_options, &agent
				);
			}
			return agent.failures;
		}

		Attachment* create_initial_node_attachment() override {
			paths_without_deadline_misses = 0;
			did_update_failures = false;
			return new Attachment_failure_search();
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_failure_search * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto new_attachment = new Attachment_failure_search();
			new_attachment->job_sequence = parent_attachment->job_sequence.extended_copy(next_job.get_job_index());
			new_attachment->has_missed_deadline = parent_attachment->has_missed_deadline;
			return new_attachment;
		}

		void merge_node_attachments(
				Global::Schedule_node<Time>* destination_node,
				const Global::Schedule_node<Time> &parent_node,
				Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_failure_search * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto destination_attachment = dynamic_cast<Attachment_failure_search*>(destination_node->attachment);
			assert(destination_attachment);

			std::cout << "performed a failure merge\n";
			destination_attachment->job_sequence.merge(parent_attachment->job_sequence.extended_copy(next_job.get_job_index()));
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Attachment_failure_search * const>(failed_node.attachment);
			assert(attachment);

			if (!attachment->has_missed_deadline) {
				attachment->has_missed_deadline = true;
				add_failure(attachment->job_sequence);
			}
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			const auto attachment = dynamic_cast<Attachment_failure_search* const>(dead_node.attachment);
			assert(attachment);

			if (!attachment->has_missed_deadline) {
				attachment->has_missed_deadline = true;

				add_failure(attachment->job_sequence);
			}
		}

		void finished_node(const Global::Schedule_node<Time> &dead_node) override {
			paths_without_deadline_misses += 1;
		}

		bool allow_merge(const Global::Schedule_node<Time> &node, const Global::Schedule_node<Time> &other) override {
			const auto attachment1 = dynamic_cast<Attachment_failure_search*>(node.attachment);
			assert(attachment1);

			const auto attachment2 = dynamic_cast<Attachment_failure_search*>(other.attachment);
			assert(attachment2);

			// Don't allow the graph to merge nodes that missed a deadline with nodes that didn't
			return attachment1->has_missed_deadline == attachment2->has_missed_deadline;
		}

		// TODO Implement this

//		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
//			const auto attachment = dynamic_cast<Attachment_job_sequence*>(node.attachment);
//			assert(attachment);
//
//			auto new_job_ids = attachment->chosen_job_ids;
//			new_job_ids.push_back(next_job.get_id());
//
//			for (const auto failure : this->failures) {
//				if (failure.chosen_job_ids == new_job_ids) {
//					return false;
//				}
//			}
//
//			return true;
//		}
	};

	template <class Time> class Agent_failure_alternative_search final : public Agent_job_sequence_history<Time> {
	public:
		std::vector<Job_sequence> failures;
		std::vector<std::vector<JobID>> alternatives;

		explicit Agent_failure_alternative_search(const std::vector<Job_sequence>& failures) : failures(failures) {
			for (int index = 0; index < failures.size(); index++) alternatives.emplace_back(); // TODO What's this?
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence*>(node.attachment);
			assert(attachment);

			// auto next_job_ids = attachment->chosen_job_ids;
			// next_job_ids.push_back(next_job.get_id());

			// TODO Convert this
//			for (int index = 0; index < failures.size(); index++) {
//				const auto &failure = failures[index];
//				auto most_chosen_job_ids = failure.chosen_job_ids;
//				most_chosen_job_ids.pop_back();
//				if (most_chosen_job_ids == attachment->chosen_job_ids) {
//					if (failure.chosen_job_ids[failure.chosen_job_ids.size() - 1] == next_job.get_id()) return false;
//					alternatives[index].push_back(next_job.get_id());
//					return true;
//				}
//			}

			return true;
		}
	};
}

#endif
