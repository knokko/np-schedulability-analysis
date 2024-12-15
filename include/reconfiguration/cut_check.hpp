#ifndef CUT_TRIAL_H
#define CUT_TRIAL_H

#include "agent.hpp"
#include "attachment.hpp"
#include "rating_graph_cut.hpp"

#define CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF (-2)
#define CUT_CHECK_NODE_INDEX_CONTINUED_AFTER_LEAF (-4)

namespace NP::Reconfiguration {
	struct Attachment_cut_check final: Attachment {
		int node_index = 0;
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
			assert(parent_attachment->node_index >= 0 || parent_attachment->node_index == CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF);

			auto new_attachment = new Attachment_cut_check();

			if (parent_attachment->node_index == CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF) {
				new_attachment->node_index = CUT_CHECK_NODE_INDEX_CONTINUED_AFTER_LEAF;
			} else if (cut.previous_jobs->is_leaf(parent_attachment->node_index)) {
				if (leaf_index > 0) assert(leaf_index == parent_attachment->node_index);
				leaf_index = parent_attachment->node_index;

				bool is_allowed = std::find(
						cut.allowed_jobs.begin(), cut.allowed_jobs.end(), next_job.get_job_index()
				) != cut.allowed_jobs.end();
				is_allowed |= std::find(
						cut.extra_allowed_jobs.begin(), cut.extra_allowed_jobs.end(), next_job.get_job_index()
				) != cut.extra_allowed_jobs.end();
				bool is_forbidden = std::find(
						cut.forbidden_jobs.begin(), cut.forbidden_jobs.end(), next_job.get_job_index()
				) != cut.forbidden_jobs.end();
				is_forbidden |= std::find(
						cut.extra_forbidden_jobs.begin(), cut.extra_forbidden_jobs.end(), next_job.get_job_index()
				) != cut.extra_forbidden_jobs.end();
				if (!is_allowed && !is_forbidden) {
					std::cout << "job is " << next_job << " and node index is " << parent_attachment->node_index << " and #allowed jobs is " << this->cut.allowed_jobs.size() << std::endl;
				}
				assert(is_allowed || is_forbidden);
				assert(!is_allowed || !is_forbidden);

//				if (is_forbidden) did_take_cut_edge = true;
//				else new_attachment->node_index = CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF;
				if (is_allowed) new_attachment->node_index = CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF;
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

			if (attachment->node_index == CUT_CHECK_NODE_INDEX_CONTINUED_AFTER_LEAF) return; // These nodes are out-of-scope
			else has_unexpected_failure = true;
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			if (did_take_cut_edge) return;
			const auto attachment = dynamic_cast<Attachment_cut_check*>(dead_node.attachment);
			assert(attachment);

			if (attachment->node_index >= 0 || attachment->node_index == CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF) {
				has_unexpected_failure = true;
			}
		}

		bool may_potentially_forbid_jobs() override {
			return true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			if (did_take_cut_edge) return false;
			const auto attachment = dynamic_cast<Attachment_cut_check*>(node.attachment);
			assert(attachment);

			if (attachment->node_index == CUT_CHECK_NODE_INDEX_CONTINUED_AFTER_LEAF) return false;
			if (attachment->node_index == CUT_CHECK_NODE_INDEX_RIGHT_AFTER_LEAF || cut.previous_jobs->is_leaf(attachment->node_index)) return true;
			return cut.previous_jobs->can_take_job(attachment->node_index, next_job.get_job_index()) > 0;
		}
	};
}

#endif
