#ifndef RATING_GRAPH_H
#define RATING_GRAPH_H
#include <ranges>
#include <vector>

#include "agent.hpp"
#include "attachment.hpp"
#include "rating_graph_cut.hpp"

namespace NP::Reconfiguration {
	struct Rating_edge {
		const int destination_node_index;
		const Job_index taken_job;

		void print() const {
			std::cout << "  destination_node_index = " << destination_node_index << " with job index " << taken_job << std::endl;
		}
	};

	struct Rating_node {
		std::vector<Rating_edge> edges;
		float rating;

		void print() const {
			std::cout << "rating = " << rating << " and edges are: " << std::endl;
			for (const auto &edge : edges) edge.print();
		}
	};

	class Rating_graph {
	public:
		std::vector<Rating_node> nodes;

		Rating_graph() {
			nodes.push_back(Rating_node {});
		}

		int add_node(int parent_index, Job_index taken_job) {
			assert(taken_job >= 0);
			if (will_miss_deadline(parent_index, taken_job)) return parent_index;

			int child_index = nodes.size();
			nodes[parent_index].edges.push_back(Rating_edge { .destination_node_index=child_index, .taken_job=taken_job });
			nodes.push_back(Rating_node { });
			nodes[child_index].edges.push_back(Rating_edge { .destination_node_index=parent_index, .taken_job=taken_job });
			std::cout << "add_node(" << parent_index << ", " << taken_job << ") -> " << child_index << std::endl;
			return child_index;
		}

		void insert_edge(int parent_index, int child_index, Job_index taken_job) {
			assert(parent_index >= 0 && parent_index < nodes.size());
			assert(child_index >= 0 && child_index < nodes.size());
			assert(taken_job >= 0);
			assert(parent_index < child_index);
			nodes[parent_index].edges.push_back(Rating_edge { .destination_node_index=child_index, .taken_job=taken_job });
			nodes[child_index].edges.push_back(Rating_edge { .destination_node_index=parent_index, .taken_job=taken_job });
		}

		void set_missed_deadline(int node_index) {
			assert(node_index >= 0);
			assert(node_index < nodes.size());
			nodes[node_index].rating = -1.0f;
			std::cout << "set_missed_deadline " << node_index << std::endl;
		}

		void set_successful(int node_index) {
			assert(node_index >= 0 && node_index < nodes.size());
			std::cout << "set_successful " << node_index << std::endl;
			if (nodes[node_index].rating == -1.0f) return;

			bool has_children = false;
			for (const auto &edge : nodes[node_index].edges) {
				if (edge.destination_node_index > node_index) {
					nodes[edge.destination_node_index].rating = 1.0f;
					has_children = true;
				}
			}

			if (!has_children) nodes[node_index].rating = 1.0f;
		}

		bool will_miss_deadline(int parent_index, Job_index candidate_job) const {
			assert(parent_index >= 0 && parent_index < nodes.size());
			//std::cout << "parent rating is " << nodes[parent_index].rating << " and parent index is " << parent_index << std::endl;
			if (nodes[parent_index].rating == -1.0f) return true;
			for (const auto &edge : nodes[parent_index].edges) {
				if (edge.taken_job == candidate_job && nodes[edge.destination_node_index].rating == -1.0f) return true;
			}
			return false;
		}

		void compute_ratings() {
			for (int index = nodes.size() - 1; index >= 0; index--) {
				auto &node = nodes[index];
				//std::cout << "compute ratings: original rating of node " << index << " is " << node.rating << std::endl;
				if (node.rating == -1.0f) {
					node.rating = 0.0f;
					continue;
				}

				int num_children = 0;
				for (const auto &edge : node.edges) {
					if (edge.destination_node_index > index) {
						node.rating += nodes[edge.destination_node_index].rating;
						num_children += 1;
					}
				}
				if (num_children > 1) node.rating /= num_children;
			}
		}

		template<class Time> void generate_dot_file(
				const char *file_path,
				const Scheduling_problem<Time> &problem,
				const std::vector<Rating_graph_cut> &cuts
		) {
			FILE *file = fopen(file_path, "w");
			if (!file) {
				std::cout << "Failed to write to file " << file_path << std::endl;
				return;
			}

			std::vector<std::vector<int>> subgraph_node_mapping;
			subgraph_node_mapping.reserve(cuts.size());
			for (int cut_index = 0; cut_index < cuts.size(); cut_index++) {
				subgraph_node_mapping.emplace_back();
				auto &last = subgraph_node_mapping[subgraph_node_mapping.size() - 1];
				last.reserve(nodes.size());
				last.push_back(0);
				for (int node_index = 1; node_index < nodes.size(); node_index++) last.push_back(-2);
			}

			std::vector<int> should_visit_nodes;
			should_visit_nodes.push_back(2);
			for (int counter = 1; counter < nodes.size(); counter++) should_visit_nodes.push_back(0);

			fprintf(file, "strict digraph Rating {\n");

			for (int index = 0; index < nodes.size(); index++) {
				if (should_visit_nodes[index] == 0) continue;
				fprintf(file, "\tnode%u [label=%.2f", index, nodes[index].rating);
				if (nodes[index].rating == 0.0f) fprintf(file, ", color=red");
				if (nodes[index].rating == 1.0f) fprintf(file, ", color=green");
				fprintf(file, "];\n");
				if (should_visit_nodes[index] == 1) continue;
				for (const auto &edge : nodes[index].edges) {
					if (edge.destination_node_index < index) continue;
					should_visit_nodes[edge.destination_node_index] = nodes[edge.destination_node_index].rating == 1.0f ? 1 : 2;
					const auto job = problem.jobs[edge.taken_job].get_id();
					fprintf(
							file, "\tnode%u -> node%u [label=\"T%uJ%u (%zu)\"",
							index, edge.destination_node_index, job.task, job.job, edge.taken_job
					);

					for (int cut_index = 0; cut_index < cuts.size(); cut_index++) {
						int mapped_source_node = subgraph_node_mapping[cut_index][index];
						if (mapped_source_node < 0) continue;
						int mapped_destination_node = cuts[cut_index].previous_jobs->can_take_job(
								mapped_source_node, edge.taken_job
						);
						subgraph_node_mapping[cut_index][edge.destination_node_index] = mapped_destination_node;
						if (!cuts[cut_index].previous_jobs->is_leaf(mapped_source_node)) continue;

						bool is_forbidden = false;
						for (const auto forbidden_job : cuts[cut_index].forbidden_jobs) {
							if (edge.taken_job == forbidden_job) {
								is_forbidden = true;
								break;
							}
						}

						if (is_forbidden) {
							fprintf(file, ", color=red");
							break;
						}
					}

					fprintf(file, "];\n");
				}
			}

			fprintf(file, "}\n");

			fclose(file);
		}
	};

	struct Attachment_rating_node final: Attachment {
		int index = 0;
	};

	template<class Time> class Agent_rating_graph : public Agent<Time> {
		Rating_graph *rating_graph;

	public:
		static void generate(const Scheduling_problem<Time> &problem, Rating_graph &rating_graph) {
			Agent_rating_graph agent;
			agent.rating_graph = &rating_graph;

			Analysis_options test_options;
			test_options.early_exit = false;
			test_options.use_supernodes = false;

			Global::State_space<Time>::explore(problem, test_options, &agent);
			rating_graph.compute_ratings();
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
			new_attachment->index = rating_graph->add_node(parent_attachment->index, next_job.get_job_index());
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
			rating_graph->insert_edge(parent_attachment->index, child_attachment->index, next_job.get_job_index());
		}

		void missed_deadline(const Global::Schedule_node<Time> &failed_node, const Job<Time> &late_job) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(failed_node.attachment);
			assert(attachment);
			rating_graph->set_missed_deadline(attachment->index);
			//std::cout << "missed deadline of job " << late_job.get_job_index() << " at index " << attachment->index << std::endl;
		}

		void finished_node(const Global::Schedule_node<Time> &node) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);
			rating_graph->set_successful(attachment->index);
		}

		bool may_potentially_forbid_jobs() override {
			return true;
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);

			bool result = !rating_graph->will_miss_deadline(attachment->index, next_job.get_job_index());
			//std::cout << "allowed? " << result << std::endl;
			return result;
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
			return (rating_graph->nodes[destination_attachment->index].rating != -1.0) &&
				   !rating_graph->will_miss_deadline(parent_attachment->index, taken_job.get_job_index());
		}
	};
}

#endif
