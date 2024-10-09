#ifndef RECONFIGURATION_AGENT_JOB_SEQUENCE_HISTORY_H
#define RECONFIGURATION_AGENT_JOB_SEQUENCE_HISTORY_H

#include "../attachment.hpp"
#include "../agent.hpp"

namespace NP::Reconfiguration {
	template <class Time> class Agent_job_sequence_history : public Agent<Time> {
	public:
		Attachment* create_initial_node_attachment() override {
			return new Attachment_job_sequence();
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_job_sequence * const>(parent_node.attachment);
			assert(parent_attachment);

			return new Attachment_job_sequence {
					.job_sequence=parent_attachment->job_sequence.extended_copy(next_job.get_job_index())
			};
		}

		void merge_node_attachments(
				Global::Schedule_node<Time>* destination_node,
				const Global::Schedule_node<Time> &parent_node,
				Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_job_sequence * const>(parent_node.attachment);
			assert(parent_attachment);

			const auto destination_attachment = dynamic_cast<Attachment_job_sequence*>(destination_node->attachment);
			assert(destination_attachment);

			std::cout << "performed a merge\n";
			destination_attachment->job_sequence.merge(parent_attachment->job_sequence.extended_copy(next_job.get_job_index()));
		}
	};
}

#endif
