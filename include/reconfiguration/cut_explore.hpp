#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
#ifndef CUT_EXPLORE_H
#define CUT_EXPLORE_H

#include "agent.hpp"
#include "attachment.hpp"
#include "rating_graph_cut.hpp"

#define CUT_EXPLORE_NODE_INDEX_CONTINUED (-2)

namespace NP::Reconfiguration {
	struct Attachment_cut_explore final: Attachment {
		int node_index = 0;
		Job_index taken_job = -1;
		bool has_failed = false;
	};

	template<class Time> class Agent_cut_explore final: public Agent<Time> {
		Rating_graph_cut *cut = nullptr;
		int leaf_index = -1;

		std::vector<Job_index> new_good_jobs;
		std::vector<Job_index> new_bad_jobs;

		bool contains(const std::vector<Job_index> &vector, Job_index candidate) {
			return std::find(vector.begin(), vector.end(), candidate) != vector.end();
		}

		bool contains(const std::vector<Job_index> &vector, const Job<Time> &candidate) {
			return contains(vector, candidate.get_job_index());
		}

		void failed_node(const Global::Schedule_node<Time> &failed_node) {
			const auto attachment = dynamic_cast<Attachment_cut_explore*>(failed_node.attachment);
			assert(attachment);
			assert(attachment->taken_job >= 0);
			attachment->has_failed = true;
			if (contains(new_good_jobs, attachment->taken_job)) {
				new_bad_jobs.push_back(attachment->taken_job);
				new_good_jobs.erase(std::remove(
						new_good_jobs.begin(), new_good_jobs.end(), attachment->taken_job
				), new_good_jobs.end());
			}
		}
	public:
		static void explore_forbidden_jobs(Scheduling_problem<Time> &problem, Rating_graph_cut *cut) {
			assert(cut != nullptr);

			Agent_cut_explore agent;
			agent.cut = cut;

			Analysis_options test_options;
			test_options.early_exit = false;
			test_options.use_supernodes = false;

			while (true) {
				std::cout << "Start exploration\n";
				Global::State_space<Time>::explore(problem, test_options, &agent);
				if (agent.new_bad_jobs.empty()) return;

				for (const auto good_job : agent.new_good_jobs) cut->extra_allowed_jobs.push_back(good_job);
				for (const auto bad_job : agent.new_bad_jobs) cut->extra_forbidden_jobs.push_back(bad_job);
				agent.new_good_jobs.clear();
				agent.new_bad_jobs.clear();
			}
		}

		Attachment* create_initial_node_attachment() override {
			return new Attachment_cut_explore {};
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_cut_explore*>(parent_node.attachment);
			assert(parent_attachment);
			assert(parent_attachment->node_index >= 0 || parent_attachment->node_index == CUT_EXPLORE_NODE_INDEX_CONTINUED);

			auto new_attachment = new Attachment_cut_explore();

			//assert(cut != nullptr);
			if (parent_attachment->node_index == CUT_EXPLORE_NODE_INDEX_CONTINUED) {
				assert(parent_attachment->taken_job >= 0);
				new_attachment->node_index = CUT_EXPLORE_NODE_INDEX_CONTINUED;
				new_attachment->has_failed = parent_attachment->has_failed;
			} else if (cut->previous_jobs->is_leaf(parent_attachment->node_index)) {
				assert(!parent_attachment->has_failed);
				assert(parent_attachment->taken_job == -1);
				if (leaf_index >= 0) assert(leaf_index == parent_attachment->node_index);
				leaf_index = parent_attachment->node_index;

				new_attachment->node_index = CUT_EXPLORE_NODE_INDEX_CONTINUED;
				new_attachment->taken_job = next_job.get_job_index();
				std::cout << "Discovered extra job " << next_job << std::endl;
				new_good_jobs.push_back(new_attachment->taken_job);
			} else {
				assert(!parent_attachment->has_failed);
				new_attachment->node_index = cut->previous_jobs->can_take_job(
						parent_attachment->node_index, next_job.get_job_index()
				);
				assert(new_attachment->node_index > 0);
			}

			return new_attachment;
		}

		void missed_deadline(const Global::Schedule_node<Time> &missed_node, const Job<Time> &late_job) override {
			failed_node(missed_node);
		}

		void encountered_dead_end(const Global::Schedule_node<Time> &dead_node) override {
			const auto attachment = dynamic_cast<Attachment_cut_explore*>(dead_node.attachment);
			assert(attachment);
			if (attachment->node_index != CUT_EXPLORE_NODE_INDEX_CONTINUED && cut->previous_jobs->is_leaf(attachment->node_index)) return;

			failed_node(dead_node);
		}

		bool may_potentially_forbid_jobs() override {
			return true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_cut_explore*>(node.attachment);
			assert(attachment);
			assert(attachment->node_index >= 0 || attachment->node_index == CUT_EXPLORE_NODE_INDEX_CONTINUED);

			if (attachment->has_failed) return false;
			if (attachment->node_index == CUT_EXPLORE_NODE_INDEX_CONTINUED) return true;

			if (cut->previous_jobs->is_leaf(attachment->node_index)) {
				return !contains(cut->forbidden_jobs, next_job) && !contains(cut->extra_forbidden_jobs, next_job);
			} else {
				return cut->previous_jobs->can_take_job(
						attachment->node_index, next_job.get_job_index()
				) > 0;
			}
		}

		bool allow_merge(
				const Global::Schedule_node<Time> &parent_node,
				const Job<Time> &taken_job,
				const Global::Schedule_node<Time> &destination_node
		) override {
			const auto attachment1 = dynamic_cast<Attachment_cut_explore*>(parent_node.attachment);
			const auto attachment2 = dynamic_cast<Attachment_cut_explore*>(destination_node.attachment);
			if (attachment1->has_failed || attachment2->has_failed) return false;
			return attachment1->taken_job == attachment2->taken_job;
		}
	};
}

#endif

#pragma clang diagnostic pop