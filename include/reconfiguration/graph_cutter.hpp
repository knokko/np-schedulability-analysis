#ifndef TREE_CUTTER_H
#define TREE_CUTTER_H

#include <vector>

#include "rating_graph.hpp"
#include "index_collection.hpp"

namespace NP::Reconfiguration {
	struct Rating_graph_cut {
		std::unique_ptr<Index_collection> previous_jobs;
		Job_index forbidden_job;
	};

	std::vector<Rating_graph_cut> cut_rating_graph(Rating_graph &graph) {
		struct Cut_builder {
			int node_index;
			std::vector<Job_index> forbidden_jobs;
		};
		std::vector<Cut_builder> cut_builders;

		std::vector<bool> has_visited;
		has_visited.reserve(graph.nodes.size());
		for (int counter = 0; counter < graph.nodes.size(); counter++) has_visited.push_back(false);

		struct Node {
			const int index;
			int next_edge_index;
			float largest_child_rating;
		};

		std::vector<Node> branch;
		branch.push_back(Node { .index=0 });

		while (branch.size() > 0) {
			int branch_index = branch.size() - 1;
			int node_index = branch[branch_index].index;
			int edge_index = branch[branch_index].next_edge_index;

			auto node = graph.nodes[node_index];

			assert(node.rating > 0.0f);
			if (node.rating == 1.0 || has_visited[node_index] || edge_index >= node.edges.size()) {
				has_visited[node_index] = true;
				branch.pop_back();
				continue;
			}

			if (edge_index == 0) {
				for (const auto &edge : node.edges) {
					if (edge.destination_node_index > node_index) {
						branch[branch_index].largest_child_rating = std::max(
								branch[branch_index].largest_child_rating, graph.nodes[edge.destination_node_index].rating
						);
					}
				}
			}

			branch[branch_index].next_edge_index += 1;
			auto current_edge = node.edges[edge_index];
			auto destination = graph.nodes[current_edge.destination_node_index];
			if (destination.rating == 1.0f || current_edge.destination_node_index < node_index) continue; // TODO Check if edge is already cut by a similar node in previous iterations

			if (destination.rating < 0.4f && destination.rating < branch[branch_index].largest_child_rating - 0.4f) { // TODO Experiment with thresholds
				bool add_new = true;
				for (auto &cut : cut_builders) {
					if (cut.node_index == node_index) {
						add_new = false;
						cut.forbidden_jobs.push_back(current_edge.taken_job);
					}
				}

				if (add_new) {
					cut_builders.push_back(Cut_builder { .node_index = node_index });
					cut_builders[cut_builders.size() - 1].forbidden_jobs.push_back(current_edge.taken_job);
				}
				continue;
			}

			branch.push_back(Node { .index=current_edge.destination_node_index });
		}

		std::vector<Rating_graph_cut> cuts;
		for (const auto &builder : cut_builders) {
			// TODO Support multiple forbidden jobs
			auto previous_jobs = std::make_unique<Index_collection>();
			int backtrack_node_index = builder.node_index;

			while (backtrack_node_index > 0) {
				// TODO Support graph of previous jobs
				for (const auto &edge : graph.nodes[backtrack_node_index].edges) {
					if (edge.destination_node_index < backtrack_node_index) {
						backtrack_node_index = edge.destination_node_index;
						previous_jobs->insert(edge.taken_job);
						break;
					}
				}
			}
			cuts.push_back(Rating_graph_cut { .previous_jobs=std::move(previous_jobs), .forbidden_job=builder.forbidden_jobs[0] });
		}

		return cuts;
	}
}

#endif
