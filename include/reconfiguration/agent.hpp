#ifndef RECONFIGURATION_AGENT_H
#define RECONFIGURATION_AGENT_H
#include <optional>

#include "global/state.hpp"
#include "attachment.hpp"
#include "jobs.hpp"

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

		virtual bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) {
			return true;
		}
	};

	struct FailedSequence {
		std::vector<JobID> chosen_job_ids;
		std::optional<JobID> missed_job_id;

		void print() const {
			for (const auto job_id : chosen_job_ids) std::cout << job_id << ", ";
			if (missed_job_id.has_value()) std::cout << "(missed " << *missed_job_id << ")\n";
			else std::cout << "dead\n";
		}

		[[nodiscard]] bool is_prefix_of(const FailedSequence &other) const {
			if (other.chosen_job_ids.size() <= chosen_job_ids.size()) return false;

			for (int index = 0; index < chosen_job_ids.size(); index++) {
				if (other.chosen_job_ids[index] != chosen_job_ids[index]) return false;
			}

			//return missed_job_id == other.chosen_job_ids[chosen_job_ids.size()];
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

	template <class Time> class Agent_simple_failure_search : public Agent_job_sequence_history<Time> {
		void add_failure(FailedSequence failure) {
			for (int index = 0; index < failures.size(); index++) {
				if (failure.is_prefix_of(failures[index])) {
					failures[index] = failures[failures.size() - 1];
					failures.pop_back();
				}
			}
			failures.push_back(failure);
		}
	public:
		bool instant_failure = false;
		std::vector<FailedSequence> failures;

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence * const>(failed_node.attachment);
			assert(attachment);

			auto chosen_job_ids = attachment->chosen_job_ids;
			chosen_job_ids.push_back(late_job.get_id());

			add_failure(FailedSequence {
				.chosen_job_ids=chosen_job_ids,
				.missed_job_id=late_job.get_id()
			});

			std::cout << "Missed deadline from " << attachment->chosen_job_ids[attachment->chosen_job_ids.size() - 1] << " to " << late_job.get_id() << "\n";
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence * const>(dead_node.attachment);
			assert(attachment);

			std::cout << "Encountered dead end after " << attachment->chosen_job_ids.size() << ": ";
			for (const auto job : attachment->chosen_job_ids) std::cout << job << ", ";
			std::cout << "\n";

			if (attachment->chosen_job_ids.empty()) instant_failure = true;
			else {
                add_failure(FailedSequence {
                    .chosen_job_ids=attachment->chosen_job_ids,
                    .missed_job_id=std::nullopt
                });
			}
		}
	};

	template <class Time> class Agent_exhaustive_failure_search : public Agent_simple_failure_search<Time> {
		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_job_sequence*>(node.attachment);
			assert(attachment);

			auto new_job_ids = attachment->chosen_job_ids;
			new_job_ids.push_back(next_job.get_id());

			for (const auto failure : this->failures) {
				// // TODO Hm... fix this
				// auto fsize = failure.chosen_job_ids.size();
				// auto asize = attachment->chosen_job_ids.size();
				// if ((fsize == asize && failure.missed_job_id.has_value()) || (fsize == asize + 1 && !failure.missed_job_id.has_value())) {
				// 	bool matched = true;
				// 	for (int index = 0; index < asize; index++) {
				// 		if (failure.chosen_job_ids[index] != attachment->chosen_job_ids[index]) {
				// 			matched = false;
				// 			break;
				// 		}
				// 	}
				//
				// 	if (!matched) continue;
				//
				// 	if (failure.missed_job_id.has_value()) matched = *failure.missed_job_id == ehm;
				// }
				if (failure.chosen_job_ids == new_job_ids/* && failure.missed_job_id == next_job.get_id()*/) {
					std::cout << "Prevented transition to " << next_job.get_id() << "\n";
					// TODO check first case
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
