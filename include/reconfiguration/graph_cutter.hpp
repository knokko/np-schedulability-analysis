#ifndef TREE_CUTTER_H
#define TREE_CUTTER_H

#include <vector>

#include "rating_graph.hpp"
#include "index_collection.hpp"
#include "sub_graph.hpp"

namespace NP::Reconfiguration {
	struct Rating_graph_cut {
		std::shared_ptr<Sub_graph> previous_jobs;
		std::vector<Job_index> forbidden_jobs;
		std::vector<Job_index> allowed_jobs;
	};

	static std::vector<Rating_graph_cut> cut_rating_graph(Rating_graph &graph) {
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

			if (destination.rating < branch[branch_index].largest_child_rating) {
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
			Sub_graph previous_jobs;
			std::vector<int> nodes_to_visit;
			nodes_to_visit.push_back(builder.node_index);

			std::vector<int> sub_graph_mapping;
			sub_graph_mapping.reserve(graph.nodes.size());
			for (int counter = 0; counter < graph.nodes.size(); counter++) sub_graph_mapping.push_back(-1);
			sub_graph_mapping[builder.node_index] = 0;

			while (nodes_to_visit.size() > 0) {
				int current_node = nodes_to_visit[nodes_to_visit.size() - 1];
				nodes_to_visit.pop_back();

				int mapped_current_node = sub_graph_mapping[current_node];
				assert(mapped_current_node >= 0);

				for (const auto &edge : graph.nodes[current_node].edges) {
					if (edge.destination_node_index < current_node) {
						int destination_node = sub_graph_mapping[edge.destination_node_index];
						if (destination_node >= 0) {
							previous_jobs.add_edge_between_existing_nodes(
									mapped_current_node, destination_node, edge.taken_job
							);
						} else {
							destination_node = previous_jobs.add_edge_to_new_node(mapped_current_node, edge.taken_job);
							assert(destination_node > 0);
							sub_graph_mapping[edge.destination_node_index] = destination_node;
							nodes_to_visit.push_back(edge.destination_node_index);
						}
					}
				}
			}

			std::vector<Job_index> allowed_jobs;
			for (const auto &edge : graph.nodes[builder.node_index].edges) {
				if (edge.destination_node_index > builder.node_index) {
					bool is_forbidden = false;
					for (const auto forbidden_job : builder.forbidden_jobs) {
						if (edge.taken_job == forbidden_job) {
							is_forbidden = true;
							break;
						}
					}

					if (!is_forbidden) allowed_jobs.push_back(edge.taken_job);
				}
			}
			cuts.push_back(Rating_graph_cut {
				.previous_jobs=std::make_unique<Sub_graph>(previous_jobs.reversed()),
				.forbidden_jobs=builder.forbidden_jobs,
				.allowed_jobs=allowed_jobs
			});
		}

		return cuts;
	}
}

#endif
