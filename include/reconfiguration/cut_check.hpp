#ifndef CUT_TRIAL_H
#define CUT_TRIAL_H

#include "agent.hpp"
#include "attachment.hpp"
#include "graph_cutter.hpp"

namespace NP::Reconfiguration {
	struct Attachment_cut_check final: Attachment {
		int node_index;
	};
	template<class Time> class Agent_cut_check : public Agent<Time> {
		Rating_graph_cut cut;
		int leaf_index = 0;
		bool did_take_cut_edge = false;
		bool has_unexpected_failure = false;
	public:
		static int was_cut_performed(Scheduling_problem<Time> &problem, Rating_graph_cut cut) {
			Agent_cut_check agent;
			agent.cut = cut;

			Analysis_options test_options;
			test_options.early_exit = true;
			test_options.use_supernodes = false;

			Global::State_space<Time>::explore(problem, test_options, &agent);
			if (agent.did_take_cut_edge) return 1;
			if (agent.has_unexpected_failure) return 2;
			return 0;
		}

		Attachment* create_initial_node_attachment() override {
			return new Attachment_cut_check {};
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			if (did_take_cut_edge) return nullptr;
			const auto parent_attachment = dynamic_cast<Attachment_cut_check*>(parent_node.attachment);
			assert(parent_attachment);
			assert(parent_attachment->node_index >= -2);
			assert(parent_attachment->node_index != -1);

			auto new_attachment = new Attachment_cut_check();

			if (parent_attachment->node_index == -2) {
				new_attachment->node_index = -4;
			} else if (cut.previous_jobs->is_leaf(parent_attachment->node_index)) {
				if (leaf_index > 0) assert(leaf_index == parent_attachment->node_index);
				leaf_index = parent_attachment->node_index;

				bool is_allowed = std::find(cut.allowed_jobs.begin(), cut.allowed_jobs.end(), next_job.get_job_index()) != cut.allowed_jobs.end();
				bool is_forbidden = std::find(cut.forbidden_jobs.begin(), cut.forbidden_jobs.end(), next_job.get_job_index()) != cut.forbidden_jobs.end();
				assert(is_allowed || is_forbidden);
				assert(!is_allowed || !is_forbidden);

				if (is_allowed) new_attachment->node_index = -2;
				else did_take_cut_edge = true;
			} else {
				new_attachment->node_index = cut.previous_jobs->can_take_job(parent_attachment->node_index, next_job.get_job_index());
				assert(new_attachment->node_index > 0);
			}

			return new_attachment;
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			if (did_take_cut_edge) return;
			const auto attachment = dynamic_cast<Attachment_cut_check*>(failed_node.attachment);
			assert(attachment);

			if (attachment->node_index == -4) return; // These nodes are out-of-scope
			else has_unexpected_failure = true;
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			if (did_take_cut_edge) return;
			const auto attachment = dynamic_cast<Attachment_cut_check*>(dead_node.attachment);
			assert(attachment);

			if (attachment->node_index >= 0 || attachment->node_index == -2) has_unexpected_failure = true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			if (did_take_cut_edge) return false;
			const auto attachment = dynamic_cast<Attachment_cut_check*>(node.attachment);
			assert(attachment);

			if (attachment->node_index == -4) return false;
			if (attachment->node_index == -2 || cut.previous_jobs->is_leaf(attachment->node_index)) return true;
			return cut.previous_jobs.get()->can_take_job(attachment->node_index, next_job.get_job_index()) > 0;
		}
	};
}

#endif
