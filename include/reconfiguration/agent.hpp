#ifndef RECONFIGURATION_AGENT_H
#define RECONFIGURATION_AGENT_H

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

		virtual void merge_node_attachments(
			Global::Schedule_node<Time>* destination_node,
			const Global::Schedule_node<Time> &parent_node,
			Job<Time> next_job
		) { }

		virtual void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) {
			std::cout << "Missed deadline of job " << late_job.get_job_id() << "\n";
		}

		virtual void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) {
			//std::cout << "Encountered dead end\n";
		}

		virtual void encountered_leaf_node(const Global::Schedule_node<Time> &dead_node) {
			//std::cout << "Finished node\n";
		}

		virtual bool may_potentially_forbid_jobs() {
			return false;
		}

		virtual bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) {
			return true;
		}

		virtual bool allow_merge(
				const Global::Schedule_node<Time> &parent_node,
				const Job<Time> &taken_job,
				const Global::Schedule_node<Time> &destination_node
		) {
			//std::cout << "Allowing job merge\n";
			return true;
		}
	};
}

#endif
