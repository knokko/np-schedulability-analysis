#ifndef RATING_TREE_H
#define RATING_TREE_H
#include <ranges>
#include <vector>

#include "agent.hpp"
#include "attachment.hpp"

namespace NP::Reconfiguration {
	struct Rating_edge {
		const int child_index;
		const Job_index taken_job;

		void print() const {
			std::cout << "  child_index = " << child_index << " with job index " << taken_job << std::endl;
		}
	};

	struct Rating_node {
		std::vector<Rating_edge> children;
		float rating;

		void print() const {
			std::cout << "rating = " << rating << " and child edges are: " << std::endl;
			for (const auto & child : children) child.print();
		}
	};

	class Rating_tree {
	public:
		std::vector<Rating_node> nodes;

		Rating_tree() {
			nodes.push_back(Rating_node {});
		}

		int add_node(int parent_index, Job_index taken_job) {
			assert(taken_job >= 0);
			if (will_miss_deadline(parent_index, taken_job)) return parent_index;

			int child_index = nodes.size();
			std::cout << "Add edge from " << parent_index << " to " << child_index << " with job " << taken_job << std::endl;
			nodes[parent_index].children.push_back(Rating_edge { .child_index=child_index, .taken_job=taken_job });
			nodes.push_back(Rating_node { });
			return child_index;
		}

		void insert_edge(int parent_index, int child_index, Job_index taken_job) {
			assert(parent_index >= 0);
			assert(child_index >= 0);
			assert(taken_job >= 0);
			assert(parent_index < nodes.size());
			assert(child_index < nodes.size());
			nodes[parent_index].children.push_back(Rating_edge { .child_index=child_index, .taken_job=taken_job });
		}

		void set_missed_deadline(int node_index) {
			assert(node_index >= 0);
			assert(node_index < nodes.size());
			nodes[node_index].rating = -1.0f;
		}

		void set_successful(int node_index) {
			assert(node_index >= 0);
			assert(node_index < nodes.size());
			if (nodes[node_index].children.size() == 0) {
				nodes[node_index].rating = 1.0f;
			}

			for (const auto &edge : nodes[node_index].children) {
				nodes[edge.child_index].rating = 1.0f;
			}
		}

		bool will_miss_deadline(int parent_index, Job_index candidate_job) const {
			assert(node_index >= 0);
			assert(node_index < nodes.size());
			if (nodes[parent_index].rating == -1.0f) return true;
			for (const auto &edge : nodes[parent_index].children) {
				if (edge.child_index == candidate_job && nodes[edge.child_index].rating == -1.0f) return true;
			}
			return false;
		}

		void compute_ratings() {
			for (auto &node : std::ranges::views::reverse(nodes)) {
				// rating == -1 implies that the node misses a deadline, so its rating should just be 0
				if (node.rating == -1.0f) {
					node.rating = 0.0f;
					node.children.clear();
				}

				for (const auto &edge : node.children) {
					node.rating += nodes[edge.child_index].rating;
				}
				if (node.children.size() > 1) node.rating /= node.children.size();
			}
		}
	};

	struct Attachment_rating_node final: Attachment {
		int index;
	};

	template<class Time> class Agent_rating_tree : public Agent<Time> {
		Rating_tree *rating_tree;

	public:
		static void generate(Scheduling_problem<Time> &problem, Rating_tree &rating_tree) {
			Agent_rating_tree agent;
			agent.rating_tree = &rating_tree;

			Analysis_options test_options;
			test_options.early_exit = false;
			test_options.use_supernodes = false;

			Global::State_space<Time>::explore(problem, test_options, &agent);
			rating_tree.compute_ratings();
		}

		Attachment* create_initial_node_attachment() override {
			return new Attachment_rating_node();
		}

		Attachment* create_next_node_attachment(
				const Global::Schedule_node<Time> &parent_node, Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_rating_node*>(parent_node.attachment);
			assert(parent_attachment);
			auto new_attachment = new Attachment_rating_node();
			new_attachment->index = rating_tree->add_node(parent_attachment->index, next_job.get_job_index());
			return new_attachment;
		}

		void merge_node_attachments(
				Global::Schedule_node<Time>* destination_node,
				const Global::Schedule_node<Time> &parent_node,
				Job<Time> next_job
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_rating_node*>(parent_node.attachment);
			assert(parent_attachment);
			const auto child_attachment = dynamic_cast<Attachment_rating_node*>(destination_node->attachment);
			assert(child_attachment);
			rating_tree->insert_edge(parent_attachment->index, child_attachment->index, next_job.get_job_index());
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(failed_node.attachment);
			assert(attachment);
			rating_tree->set_missed_deadline(attachment->index);
			std::cout << "missed deadline of job " << late_job.get_job_index() << " at index " << attachment->index << std::endl;
		}

		void finished_node(const Global::Schedule_node<Time> &node) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);
			rating_tree->set_successful(attachment->index);
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);

			return !rating_tree->will_miss_deadline(attachment->index, next_job.get_job_index());
		}

		bool allow_merge(
				const Global::Schedule_node<Time> &parent_node,
				const Job<Time> &taken_job,
				const Global::Schedule_node<Time> &destination_node
		) override {
			const auto parent_attachment = dynamic_cast<Attachment_rating_node*>(parent_node.attachment);
			assert(parent_attachment);
			const auto destination_attachment = dynamic_cast<Attachment_rating_node*>(destination_node.attachment);
			assert(destination_attachment);

			// Only allow merges when both nodes haven't missed a deadline yet
			return (rating_tree->nodes[destination_attachment->index].rating != -1.0) &&
					!rating_tree->will_miss_deadline(parent_attachment->index, taken_job.get_job_index());
		}
	};
}

#endif
