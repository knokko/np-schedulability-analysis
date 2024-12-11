#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/attempt.hpp"
#include "reconfiguration/graph_strategy.hpp"

using namespace NP;

TEST_CASE("apply_graph_strategy") {
	// TODO Stop copy-pasting this
	Global::State_space<dtime_t>::Workload jobs{
			// high-frequency task
			Job<dtime_t>{0, Interval<dtime_t>(0, 0), Interval<dtime_t>(1, 2), 10, 10, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 2), 20, 20, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 2), 30, 30, 2, 2},
			Job<dtime_t>{3, Interval<dtime_t>(30, 30), Interval<dtime_t>(1, 2), 40, 40, 3, 3},
			Job<dtime_t>{4, Interval<dtime_t>(40, 40), Interval<dtime_t>(1, 2), 50, 50, 4, 4},
			Job<dtime_t>{5, Interval<dtime_t>(50, 50), Interval<dtime_t>(1, 2), 60, 60, 5, 5},

			// middle task
			Job<dtime_t>{6, Interval<dtime_t>(0, 0), Interval<dtime_t>(7, 8), 30, 30, 6, 6},
			Job<dtime_t>{7, Interval<dtime_t>(30, 30), Interval<dtime_t>(7, 8), 60, 60, 7, 7},

			// the long task
			Job<dtime_t>{8, Interval<dtime_t>(0, 0), Interval<dtime_t>(3, 13), 60, 60, 8, 8}
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	const auto solutions = Reconfiguration::apply_graph_strategy<dtime_t>(problem);
	REQUIRE(solutions.size() == 1);

	const auto solution = dynamic_cast<Reconfiguration::Precedence_solution*>(solutions[0]);
	REQUIRE(solution);
	CHECK(solution->from == jobs[1].get_id());
	CHECK(solution->to == jobs[8].get_id());
}

TEST_CASE("TODO handle rating of 0") {
	// Both deadlines are met when job 1 runs first, but job 0 always runs first, resulting in a rating of 0...
	Global::State_space<dtime_t>::Workload jobs {
			Job<dtime_t>{0, Interval<dtime_t>(0, 0), Interval<dtime_t>(800, 800), 900, 1, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(1, 1), Interval<dtime_t>(1, 1), 50, 2, 1, 1},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	rating_graph.generate_dot_file("test_graph_strategy_rating0.dot", problem, std::vector<Reconfiguration::Rating_graph_cut>());
	REQUIRE(rating_graph.nodes[0].rating == 0.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_graph_strategy_rating0.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	// TODO Figure out solution
}
#endif
