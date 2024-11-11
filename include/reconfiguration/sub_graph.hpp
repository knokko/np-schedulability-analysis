#ifndef SUB_GRAPH_H
#define SUB_GRAPH_H

#include <vector>

#include "jobs.hpp"

namespace NP::Reconfiguration {
	struct Sub_graph_edge {
		int child_node_index;
		Job_index taken_job;
	};

	struct Sub_graph_node {
		std::vector<Sub_graph_edge> edges;
	};

	class Sub_graph {
		std::vector<Sub_graph_node> nodes;
	public:
		Sub_graph() {
			nodes.emplace_back();
		}

		int can_take_job(int start_node, Job_index job) {
			assert(start_node >= 0 && start_node < nodes.size());

			for (const auto &edge : this->nodes[start_node].edges) {
				if (edge.taken_job == job) return edge.child_node_index;
			}

			return -1;
		}

		int length() {
			int current_length = 0;
			int current_node_index = 0;

			while (!this->nodes[current_node_index].edges.empty()) {
				current_node_index = this->nodes[current_node_index].edges[0].child_node_index;
				current_length += 1;
			}

			return current_length;
		}

		bool is_leaf(int node) {
			assert(node >= 0 && node < nodes.size());
			return this->nodes[node].edges.empty();
		}

		int add_edge_to_new_node(int start_node, Job_index taken_job) {
			assert(start_node >= 0 && start_node < nodes.size());
			int end_node = nodes.size();
			nodes.emplace_back();

			nodes[start_node].edges.push_back(Sub_graph_edge {
				.child_node_index = end_node,
				.taken_job = taken_job
			});
			return end_node;
		}

		void add_edge_between_existing_nodes(int start_node, int end_node, Job_index taken_job) {
			assert(start_node >= 0 && start_node < nodes.size());
			assert(end_node >= 0 && end_node < nodes.size());
			nodes[start_node].edges.push_back(Sub_graph_edge {
				.child_node_index = end_node,
				.taken_job = taken_job,
			});
		}

		Sub_graph reversed() {
			Sub_graph result;
			result.nodes.reserve(this->nodes.size());
			for (int counter = 1; counter < this->nodes.size(); counter++) result.nodes.emplace_back();

			for (int own_node_index = 0; own_node_index < this->nodes.size(); own_node_index++) {
				for (const auto &edge : this->nodes[own_node_index].edges) {
					result.add_edge_between_existing_nodes(
							this->nodes.size() - edge.child_node_index - 1,
							this->nodes.size() - own_node_index - 1,
							edge.taken_job
					);
				}
			}

			return result;
		}
	};
}

#endif
