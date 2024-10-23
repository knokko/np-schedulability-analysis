#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/attempt_generator.hpp"
#include "reconfiguration/cut_check.hpp"
#include "reconfiguration/cut_test.hpp"
#include "reconfiguration/graph_cutter.hpp"
#include "reconfiguration/rating_graph.hpp"
#include "reconfiguration/sub_graph.hpp"

using namespace NP;

int get_edge_destination(Reconfiguration::Rating_graph &rating_graph, int node_index, Job_index taken_job) {
	const auto node = rating_graph.nodes[node_index];
	for (const auto &edge : node.edges) {
		if (edge.taken_job == taken_job) return edge.destination_node_index;
	}
	REQUIRE(false);
	return 0;
}

TEST_CASE("Rating graph + cutter") {
	Global::State_space<dtime_t>::Workload jobs{
			// high-frequency task
			Job<dtime_t>{0, Interval<dtime_t>(0,  0), Interval<dtime_t>(1, 2), 10, 10, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 2), 20, 20, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 2), 30, 30, 2, 2},
			Job<dtime_t>{3, Interval<dtime_t>(30, 30), Interval<dtime_t>(1, 2), 40, 40, 3, 3},
			Job<dtime_t>{4, Interval<dtime_t>(40, 40), Interval<dtime_t>(1, 2), 50, 50, 4, 4},
			Job<dtime_t>{5, Interval<dtime_t>(50, 50), Interval<dtime_t>(1, 2), 60, 60, 5, 5},

			// middle task
			Job<dtime_t>{6, Interval<dtime_t>(0,  0), Interval<dtime_t>(7, 8), 30, 30, 6, 6},
			Job<dtime_t>{7, Interval<dtime_t>(30, 30), Interval<dtime_t>(7, 8), 60, 60, 7, 7},

			// the long task
			Job<dtime_t>{8, Interval<dtime_t>(0,  0), Interval<dtime_t>(3, 13), 60, 60, 8, 8}
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);

	REQUIRE(rating_graph.nodes.size() == 11);

	// Node 0 is the root, and can only take job 0
	CHECK(rating_graph.nodes[0].rating == 0.5f);
	REQUIRE(rating_graph.nodes[0].edges.size() == 1);
	REQUIRE(get_edge_destination(rating_graph, 0, 0) == 1); // Takes job 0 to node 1

	// Node 1 can only take job 6
	CHECK(rating_graph.nodes[1].rating == 0.5f);
	REQUIRE(rating_graph.nodes[1].edges.size() == 2);
	REQUIRE(get_edge_destination(rating_graph, 1, 0) == 0); // Back-edge to node 0
	REQUIRE(get_edge_destination(rating_graph, 1, 6) == 2); // Takes job 6 to node 2

	// Node 2 can take either job 1 or job 8, where job 8 is a poor choice
	CHECK(rating_graph.nodes[2].rating == 0.5f);
	REQUIRE(rating_graph.nodes[2].edges.size() == 3);
	CHECK(get_edge_destination(rating_graph, 2, 6) == 1); // Back-edge to node 1

	int failed_node_index = get_edge_destination(rating_graph, 2, 8);
	CHECK(rating_graph.nodes[failed_node_index].rating == 0.0);
	REQUIRE(rating_graph.nodes[failed_node_index].edges.size() == 1);
	CHECK(get_edge_destination(rating_graph, failed_node_index, 8) == 2); // Back-edge to node 2

	int right_node_index = get_edge_destination(rating_graph, 2, 1);
	CHECK(rating_graph.nodes[right_node_index].rating == 1.0);
	REQUIRE(rating_graph.nodes[right_node_index].edges.size() == 2);
	CHECK(get_edge_destination(rating_graph, right_node_index, 1) == 2); // Back-edge to node 2
	CHECK(get_edge_destination(rating_graph, right_node_index, 8) == 5);

	for (int index = 5; index < rating_graph.nodes.size(); index++) {
		CHECK(rating_graph.nodes[index].rating == 1.0);

		if (index != rating_graph.nodes.size() - 1) {
			REQUIRE(rating_graph.nodes[index].edges.size() == 2);
			for (const auto &edge : rating_graph.nodes[index].edges) {
				if (index > 5 && edge.destination_node_index < index) CHECK(edge.destination_node_index == index - 1);
				if (edge.destination_node_index > index) CHECK(edge.destination_node_index == index + 1);
			}
		} else {
			REQUIRE(rating_graph.nodes[index].edges.size() == 1);
			CHECK(get_edge_destination(rating_graph, index, 5) == 9);
		}
	}

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	REQUIRE(cuts.size() == 1);
	auto cut = cuts[0];
	CHECK(cut.previous_jobs->length() == 2);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	CHECK(cut.forbidden_jobs[0] == 8);
	REQUIRE(cut.allowed_jobs.size() == 1);
	CHECK(cut.allowed_jobs[0] == 1);

	auto &path = cut.previous_jobs;
	int node1 = path->can_take_job(0, 0);
	REQUIRE(node1 >= 0);
	int node2 = path->can_take_job(node1, 6);
	REQUIRE(node2 >= 0);

	// No more other paths
	for (int job = 0; job < 10; job++) CHECK(path->can_take_job(node2, job) == -1);
	std::vector<Reconfiguration::Rating_graph_cut> no_cuts;

	// was_cut_performed should return 1 since we didn't fix the cut
	CHECK(Reconfiguration::Agent_cut_check<dtime_t>::was_cut_performed(problem, cuts[0]) == 1);
	auto test_result_failed = Reconfiguration::Agent_cut_test<dtime_t>::perform(problem, no_cuts);
	CHECK(test_result_failed.has_unexpected_failures);
	CHECK(test_result_failed.fixed_cut_indices.empty());

	// Sanity check: if the cut is avoided, the problem becomes schedulable
	auto test_result_avoided = Reconfiguration::Agent_cut_test<dtime_t>::perform(problem, cuts);
	CHECK(!test_result_avoided.has_unexpected_failures);
	CHECK(test_result_avoided.fixed_cut_indices.empty());

	// fix the cut, which should cause was_cut_performed to return 0
	problem.prec.push_back(Precedence_constraint(problem.jobs[1].get_id(), problem.jobs[8].get_id(), Interval<dtime_t>(0, 0)));
	validate_prec_cstrnts(problem.prec, problem.jobs);
	CHECK(Reconfiguration::Agent_cut_check<dtime_t>::was_cut_performed(problem, cuts[0]) == 0);

	// Introduce an additional (unexpected) error, which should cause was_cut_performed to return 2
	auto old_job6 = problem.jobs[6];
	problem.jobs[6] = Job<dtime_t>{6, Interval<dtime_t>(0,  0), Interval<dtime_t>(70, 80), 30, 30, 6, 6};
	CHECK(Reconfiguration::Agent_cut_check<dtime_t>::was_cut_performed(problem, cuts[0]) == 2);

	// Sanity check: when the problem is schedulable, no 'remaining_cuts' are needed
	problem.jobs[6] = old_job6;
	auto test_result_passed = Reconfiguration::Agent_cut_test<dtime_t>::perform(problem, no_cuts);
	CHECK(!test_result_passed.has_unexpected_failures);
	CHECK(test_result_passed.fixed_cut_indices.empty());

	auto attempts = Reconfiguration::generate_precedence_attempts<dtime_t>(cut);
	REQUIRE(attempts.size() == 1);
	auto precedence_attempt = dynamic_cast<Reconfiguration::Precedence_attempt<dtime_t>*>(attempts[0].get());
	CHECK(precedence_attempt->before == 1);
	REQUIRE(precedence_attempt->after.size() == 1);
	CHECK(precedence_attempt->after[0] == 8);
}
#endif