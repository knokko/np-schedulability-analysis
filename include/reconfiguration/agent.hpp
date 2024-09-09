#ifndef RECONFIGURATION_AGENT_H
#define RECONFIGURATION_AGENT_H
#include "global/state.hpp"
#include "attachment.hpp"
#include "jobs.hpp"

namespace NP::Reconfiguration {
	template <class Time> class Reconfiguration_agent {
	public:
		virtual ~Reconfiguration_agent() = default;

		virtual Reconfiguration_attachment* create_initial_node_attachment() { return nullptr; }

		virtual Reconfiguration_attachment* create_next_node_attachment(
			const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) { return nullptr; }

		virtual void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) {
			std::cout << "Missed deadline of job " << late_job.get_job_id() << "\n";
		}

		virtual void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) {
			std::cout << "Encountered dead end\n";
		}
	};

	struct FailedSequence {
		std::vector<unsigned long> chosen_job_ids;
		unsigned long missed_job_id;
	};

	template <class Time> class Reconfiguration_agent_job_sequence_history final : public Reconfiguration_agent<Time> {
	public:

		std::vector<FailedSequence> failures;

		Reconfiguration_attachment* create_initial_node_attachment() override {
			const auto attachment = new Reconfiguration_attachment_job_sequence();
			attachment->chosen_job_ids = std::vector<unsigned long>();
			return attachment;
		}

		Reconfiguration_attachment* create_next_node_attachment(
			const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Reconfiguration_attachment_job_sequence * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto new_attachment = new Reconfiguration_attachment_job_sequence();
			new_attachment->chosen_job_ids = parent_attachment->chosen_job_ids;
			new_attachment->chosen_job_ids.push_back(next_job.get_job_id());
			return new_attachment;
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Reconfiguration_attachment_job_sequence * const>(failed_node.attachment);
			assert(attachment);

			failures.push_back(FailedSequence {
				.chosen_job_ids=attachment->chosen_job_ids,
				.missed_job_id=late_job.get_job_id()
			});
		}
	};
}

#endif
