#ifndef RATING_GRAPH_H
#define RATING_GRAPH_H
#include <ranges>
#include <vector>
#include <sys/resource.h>

#include "agent.hpp"
#include "attachment.hpp"
#include "rating_graph_cut.hpp"

namespace NP::Reconfiguration {

	struct Analysis_result {
		bool schedulable;
		bool timeout;
		unsigned long long number_of_nodes, number_of_states, number_of_edges, max_width, number_of_jobs;
		double cpu_time;
		std::string graph;
		std::string response_times_csv;
	};
	const uint64_t RATING_EDGE_DESTINATION_MASK = ((static_cast<uint64_t>(1) << 40) - 1);
	const uint64_t RATING_EDGE_JOB_MASK = ((1 << 24) - 1);

	struct Rating_edge {
		const uint64_t raw;

		Rating_edge(size_t destination_node_index, size_t taken_job) : raw(
			static_cast<uint64_t>(destination_node_index) | (static_cast<uint64_t>(taken_job) << 40)
		) {
			if (destination_node_index < 0 || taken_job < 0) throw std::invalid_argument("negative");
			if (destination_node_index != (destination_node_index & RATING_EDGE_DESTINATION_MASK)) throw std::invalid_argument("destination node");
			if (taken_job != (taken_job & RATING_EDGE_JOB_MASK)) throw std::invalid_argument("taken job");
		}

		size_t get_destination_node_index() const {
			return static_cast<size_t>(raw & RATING_EDGE_DESTINATION_MASK);
		}

		size_t get_taken_job() const {
			return static_cast<size_t>((raw >> 40) & RATING_EDGE_JOB_MASK);
		}

		void print() const {
			std::cout << "  destination_node_index = " << get_destination_node_index() << " with job index " << get_taken_job() << std::endl;
		} // TODO Measure performance
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
		std::vector<Rating_edge> edges;

		Rating_graph() {
			nodes.push_back(Rating_node {});
		}

		int add_node(int parent_index, Job_index taken_job) {
			assert(taken_job >= 0);
			if (nodes[parent_index].rating == -1.0f) return parent_index;

			int child_index = nodes.size();
			nodes[parent_index].edges.push_back(Rating_edge(child_index, taken_job));
			nodes.push_back(Rating_node { });
			nodes[child_index].edges.push_back(Rating_edge(parent_index, taken_job));
			return child_index;
		}

		void insert_edge(int parent_index, int child_index, Job_index taken_job) {
			assert(parent_index >= 0 && parent_index < nodes.size());
			assert(child_index >= 0 && child_index < nodes.size());
			assert(taken_job >= 0);
			assert(parent_index < child_index);
			nodes[parent_index].edges.push_back(Rating_edge(child_index, taken_job));
			nodes[child_index].edges.push_back(Rating_edge(parent_index, taken_job));
		}

		void set_missed_deadline(int node_index) {
			assert(node_index >= 0);
			assert(node_index < nodes.size());
			std::cout << "missed deadline of " << node_index << std::endl;
			nodes[node_index].rating = -1.0f;
		}

		void mark_as_leaf_node(int node_index) {
			std::cout << "mark node " << node_index << " as leaf node\n";
			assert(node_index >= 0 && node_index < nodes.size());
			if (nodes[node_index].rating == -1.0f) return;
			nodes[node_index].rating = 1.0f;
		}

		void compute_ratings() {
			for (int index = nodes.size() - 1; index >= 0; index--) {
				auto &node = nodes[index];
				if (node.rating == -1.0f) {
					node.rating = 0.0f;
					continue;
				}

				int num_children = 0;
				for (const auto &edge : node.edges) {
					if (edge.get_destination_node_index() > index) {
						node.rating += nodes[edge.get_destination_node_index()].rating;
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
					if (edge.get_destination_node_index() < index) continue;
					should_visit_nodes[edge.get_destination_node_index()] = nodes[edge.get_destination_node_index()].rating == 1.0f ? 1 : 2;
					const auto job = problem.jobs[edge.get_taken_job()].get_id();
					fprintf(
							file, "\tnode%u -> node%lu [label=\"T%luJ%lu (%zu)\"",
							index, edge.get_destination_node_index(), job.task, job.job, edge.get_taken_job()
					);

					for (int cut_index = 0; cut_index < cuts.size(); cut_index++) {
						int mapped_source_node = subgraph_node_mapping[cut_index][index];
						if (mapped_source_node < 0) continue;
						int mapped_destination_node = cuts[cut_index].previous_jobs->can_take_job(
								mapped_source_node, edge.get_taken_job()
						);
						subgraph_node_mapping[cut_index][edge.get_destination_node_index()] = mapped_destination_node;
						if (!cuts[cut_index].previous_jobs->is_leaf(mapped_source_node)) continue;

						bool is_forbidden = false;
						for (const auto forbidden_job : cuts[cut_index].forbidden_jobs) {
							if (edge.get_taken_job() == forbidden_job) {
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

			auto space = Global::State_space<Time>::explore(problem, test_options, &agent);
			rating_graph.compute_ratings();
			Analysis_result result = Analysis_result{
				space->is_schedulable(),
				space->was_timed_out(),
				space->number_of_nodes(),
				space->number_of_states(),
				space->number_of_edges(),
				space->max_exploration_front_width(),
				(unsigned long)(problem.jobs.size()),
				space->get_cpu_time(),
				"",
				""
			};
			delete space;
			struct rusage u;
			long mem_used = 0;

			std::cout << "node size is " << sizeof(Rating_node) << " and edge size is " << sizeof(Rating_edge) << std::endl;
			std::cout << "so estimated size is " << result.number_of_nodes * sizeof(Rating_node) << " and " << result.number_of_edges * sizeof(Rating_edge) << std::endl;
			if (getrusage(RUSAGE_SELF, &u) == 0)
				mem_used = u.ru_maxrss;
			std::cout << ",  " << result.number_of_jobs
			  << ",  " << result.number_of_nodes
		          << ",  " << result.number_of_states
		          << ",  " << result.number_of_edges
		          << ",  " << result.max_width
		          << ",  " << std::fixed << result.cpu_time
		          << ",  " << ((double) mem_used) / (1024.0)
		          << ",  " << (int) result.timeout

		          << std::endl;
			
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

		void mark_as_leaf_node(const Global::Schedule_node<Time> &node) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);
			rating_graph->mark_as_leaf_node(attachment->index);
		}

		bool is_allowed(const Global::Schedule_node<Time> &node, const Job<Time> &next_job) override {
			const auto attachment = dynamic_cast<Attachment_rating_node*>(node.attachment);
			assert(attachment);

			bool result = rating_graph->nodes[attachment->index].rating != 1.0f;
			assert(result);
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
			return rating_graph->nodes[destination_attachment->index].rating != -1.0f &&
				   rating_graph->nodes[parent_attachment->index].rating != 1.0f;
		}
	};
}

#endif
