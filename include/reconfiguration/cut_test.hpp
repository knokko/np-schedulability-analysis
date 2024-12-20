#ifndef CUT_TEST_H
#define CUT_TEST_H

#include "agent.hpp"
#include "attachment.hpp"
#include "graph_cutter.hpp"

namespace NP::Reconfiguration {
	struct Cut_test_result {
		bool has_unexpected_failures = false;
		std::vector<size_t> fixed_cut_indices;
	};

	struct Attachment_cut_test final: Attachment {
		std::vector<int> node_indices;
	};

	template<class Time> class Agent_cut_test : public Agent<Time> {
		std::vector<Rating_graph_cut> remaining_cuts;
		std::vector<bool> did_intervene;
	public:
		static Cut_test_result perform(
				const Scheduling_problem<Time> &problem,
				std::vector<Rating_graph_cut> remaining_cuts
		) {
			Agent_cut_test agent;
			agent.remaining_cuts = remaining_cuts;
			agent.did_intervene.reserve(remaining_cuts.size());
			for (int counter = 0; counter < remaining_cuts.size(); counter++) agent.did_intervene.push_back(false);

			Analysis_options test_options;
			test_options.early_exit = true;
			test_options.use_supernodes = false;

			auto space = Global::State_space<Time>::explore(problem, test_options, &agent);
			Cut_test_result result;
			result.has_unexpected_failures = !space->is_schedulable();
			delete space;
			for (int index = 0; index < remaining_cuts.size(); index++) {
				if (!agent.did_intervene[index]) result.fixed_cut_indices.push_back(index);
			}
			return result;
		}

		Attachment* create_initial_node_attachment() override {
			auto attachment = new Attachment_cut_test {};
			attachment->node_indices.reserve(remaining_cuts.size());
			for (int counter = 0; counter < remaining_cuts.size(); counter++) attachment->node_indices.push_back(0);
			return attachment;
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_cut_test*>(parent_node.attachment);
			assert(parent_attachment);

			auto new_attachment = new Attachment_cut_test {};
			new_attachment->node_indices.reserve(remaining_cuts.size());

			for (int index = 0; index < remaining_cuts.size(); index++) {
				int parent_index = parent_attachment->node_indices[index];
				if (parent_index == -1) {
					new_attachment->node_indices.push_back(-1);
					continue;
				}

				auto cut = remaining_cuts[index];
				if (cut.previous_jobs->is_leaf(parent_index)) {
					new_attachment->node_indices.push_back(-1);
					continue;
				}

				new_attachment->node_indices.push_back(cut.previous_jobs->can_take_job(
						parent_index, next_job.get_job_index()
				));
			}

			return new_attachment;
		}

		bool may_potentially_forbid_jobs() override {
			return true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_cut_test*>(node.attachment);
			assert(attachment);

			for (int index = 0; index < remaining_cuts.size(); index++) {
				int node_index = attachment->node_indices[index];
				if (node_index == -1) continue;

				auto cut = remaining_cuts[index];
				if (!cut.previous_jobs->is_leaf(node_index)) continue;

				for (auto forbidden_job : cut.forbidden_jobs) {
					if (forbidden_job == next_job.get_job_index()) {
						did_intervene[index] = true; // TODO Rework this since is_allowed shouldn't mutate the agent
						return false;
					}
				}
			}

			return true;
		}
	};
}

#endif
