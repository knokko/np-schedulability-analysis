#ifndef RECONFIGURATION_AGENT_H
#define RECONFIGURATION_AGENT_H

#include "global/state.hpp"
#include "attachment.hpp"
#include "jobs.hpp"

namespace NP {
	struct Analysis_options;
	template<class Time> class Scheduling_problem;

	namespace Global {
		template<class Time> class State_space;
	}
}

namespace NP::Reconfiguration {
	template <class Time> class Agent {
	public:
		virtual ~Agent() = default;

		virtual Attachment* create_initial_node_attachment() { return nullptr; }

		virtual Attachment* create_next_node_attachment(
			const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) { return nullptr; }

		virtual void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) {
			std::cout << "Missed deadline of job " << late_job.get_job_id() << "\n";
		}

		virtual void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) {
			std::cout << "Encountered dead end\n";
		}

		virtual void finished_node(const Global::Schedule_node<Time> &dead_node) {
			std::cout << "Finished node\n";
		}

		virtual bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) {
			return true;
		}

		virtual bool allow_merge(const Global::Schedule_node<Time> &node, const Global::Schedule_node<Time> &other) {
			std::cout << "Allowing job merge\n";
			return true;
		}
	};

	struct FailedSequence {
		std::vector<JobID> chosen_job_ids;

		void print() const {
			for (const auto job_id : chosen_job_ids) std::cout << job_id << ", ";
			std::cout << "\n";
		}

		[[nodiscard]] bool is_prefix_of(const FailedSequence &other) const {
			if (other.chosen_job_ids.size() <= chosen_job_ids.size()) return false;

			for (int index = 0; index < chosen_job_ids.size(); index++) {
				if (other.chosen_job_ids[index] != chosen_job_ids[index]) return false;
			}

			return true;
		}
	};

	template <class Time> class Agent_job_sequence_history : public Agent<Time> {
	public:
		Attachment* create_initial_node_attachment() override {
			const auto attachment = new Attachment_job_sequence();
			attachment->chosen_job_ids = std::vector<JobID>();
			return attachment;
		}

		Attachment* create_next_node_attachment(
			const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_job_sequence * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto new_attachment = new Attachment_job_sequence();
			new_attachment->chosen_job_ids = parent_attachment->chosen_job_ids;
			new_attachment->chosen_job_ids.push_back(next_job.get_id());
			return new_attachment;
		}
	};

	template <class Time> class Agent_failure_search : public Agent<Time> {
		int paths_without_deadline_misses = 0;
		std::vector<FailedSequence> failures;
		bool did_update_failures = true;

		void add_failure(const FailedSequence &failure) {
			for (int index = 0; index < failures.size(); index++) {
				if (failure.is_prefix_of(failures[index])) {
					failures[index] = failures[failures.size() - 1];
					failures.pop_back();
					did_update_failures = true;
				} else if (failures[index].is_prefix_of(failure) || failure.chosen_job_ids == failures[index].chosen_job_ids) return;
			}
			failures.push_back(failure);
			did_update_failures = true;
		}
	public:
		static std::vector<FailedSequence> find_all_failures(Scheduling_problem<Time> &problem, Analysis_options &test_options) {
			Agent_failure_search agent;
			while (agent.did_update_failures) {
				Global::State_space<Time>::explore(
					problem, test_options, &agent
				);
			}
			return agent.failures;
		}

		Attachment* create_initial_node_attachment() override {
			paths_without_deadline_misses = 0;
			did_update_failures = false;
			const auto attachment = new Attachment_failure_search();
			attachment->chosen_job_ids = std::vector<JobID>();
			attachment->has_missed_deadline = false;
			return attachment;
		}

		Attachment* create_next_node_attachment(
			const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_failure_search * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto new_attachment = new Attachment_failure_search();
			new_attachment->chosen_job_ids = parent_attachment->chosen_job_ids;
			new_attachment->chosen_job_ids.push_back(next_job.get_id());
			new_attachment->has_missed_deadline = parent_attachment->has_missed_deadline;
			return new_attachment;
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Attachment_failure_search * const>(failed_node.attachment);
			assert(attachment);

			auto chosen_job_ids = attachment->chosen_job_ids;
			chosen_job_ids.push_back(late_job.get_id());

			if (!attachment->has_missed_deadline) {
				attachment->has_missed_deadline = true;
				add_failure(FailedSequence {.chosen_job_ids=chosen_job_ids});
			}
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			const auto attachment = dynamic_cast<Attachment_failure_search* const>(dead_node.attachment);
			assert(attachment);

			if (!attachment->has_missed_deadline) {
				attachment->has_missed_deadline = true;

				add_failure(FailedSequence {.chosen_job_ids=attachment->chosen_job_ids});
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

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence*>(node.attachment);
			assert(attachment);

			auto new_job_ids = attachment->chosen_job_ids;
			new_job_ids.push_back(next_job.get_id());

			for (const auto failure : this->failures) {
				if (failure.chosen_job_ids == new_job_ids) {
					return false;
				}
			}

			return true;
		}
	};

	template <class Time> class Agent_failure_alternative_search final : public Agent_job_sequence_history<Time> {
	public:
		std::vector<FailedSequence> failures;
		std::vector<std::vector<JobID>> alternatives;

		explicit Agent_failure_alternative_search(const std::vector<FailedSequence>& failures) : failures(failures) {
			for (int index = 0; index < failures.size(); index++) alternatives.emplace_back();
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence*>(node.attachment);
			assert(attachment);

			// auto next_job_ids = attachment->chosen_job_ids;
			// next_job_ids.push_back(next_job.get_id());
			for (int index = 0; index < failures.size(); index++) {
				const auto &failure = failures[index];
				auto most_chosen_job_ids = failure.chosen_job_ids;
				most_chosen_job_ids.pop_back();
				if (most_chosen_job_ids == attachment->chosen_job_ids) {
					if (failure.chosen_job_ids[failure.chosen_job_ids.size() - 1] == next_job.get_id()) return false;
					alternatives[index].push_back(next_job.get_id());
					return true;
				}
			}

			return true;
		}
	};
}

#endif
