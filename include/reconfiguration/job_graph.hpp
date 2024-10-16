#ifndef JOB_GRAPH_H
#define JOB_GRAPH_H

#include <vector>

#include "index_collection.hpp"
#include "jobs.hpp"

namespace NP::Reconfiguration {
	struct Job_graph_edge {
		std::unique_ptr<Index_collection> jobs;
		int child_node_index;
	};

	struct Job_graph_node {
		std::vector<Job_graph_edge> edges;
		std::vector<Job_graph_edge> back_edges;
	};

	class Job_tree {
		std::vector<Job_graph_node> nodes;

	public:
		int choose_job(int node_index, Job_index job) {
			assert(node_index >= 0 && node_index < nodes.size());
			for (const auto &edge : nodes[node_index].edges) {
				if (edge.jobs->contains(job)) return edge.child_node_index;
			}
			return -1;
		}
	};
}

#endif
