#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/attempt.hpp"
#include "reconfiguration/graph_strategy.hpp"
#include "reconfiguration/verifier.hpp"

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

	const auto solution = dynamic_cast<Reconfiguration::Precedence_solution<dtime_t>*>(solutions[0]);
	REQUIRE(solution);
	CHECK(solution->from == jobs[1].get_id());
	CHECK(solution->to == jobs[8].get_id());
}

TEST_CASE("Graph strategy sanity 2") {
	Global::State_space<dtime_t>::Workload jobs {
			Job<dtime_t>{0, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 0, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 17), Interval<dtime_t>(8, 8), 50, 1, 1, 1},

			Job<dtime_t>{2, Interval<dtime_t>(10, 17), Interval<dtime_t>(100, 100), 900, 2, 2, 2},
	};

	const auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	const auto solutions = Reconfiguration::apply_graph_strategy(problem);
	REQUIRE(solutions.size() == 1);

	const auto solution = dynamic_cast<Reconfiguration::Precedence_solution<dtime_t>*>(solutions[0]);
	REQUIRE(solution);
	if (solution->from != jobs[1].get_id()) CHECK(solution->from == jobs[0].get_id());
	CHECK(solution->to == jobs[2].get_id());

	REQUIRE(Reconfiguration::verify_solution(&problem, solutions));
}

TEST_CASE("Graph strategy cut-explore first job (2)") {
	Global::State_space<dtime_t>::Workload jobs {
			// Job 0 must go first
			Job<dtime_t>{0, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 0, 0, 0},

			// If one of these goes first, then job 0 will miss its deadline
			Job<dtime_t>{1, Interval<dtime_t>(10, 16), Interval<dtime_t>(100, 100), 900, 1, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(16, 16), Interval<dtime_t>(100, 100), 900, 2, 2, 2},
	};

	const auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	const auto solutions = Reconfiguration::apply_graph_strategy<dtime_t>(problem);
	REQUIRE(solutions.size() == 2);

	for (const auto raw_solution : solutions) {
		const auto solution = dynamic_cast<Reconfiguration::Precedence_solution<dtime_t>*>(raw_solution);
		REQUIRE(solution);
		REQUIRE(solution->from.task == 0);
		REQUIRE(solution->to.task > 0);
	}

	REQUIRE(Reconfiguration::verify_solution(&problem, solutions));
}

TEST_CASE("Graph strategy cut-explore first job (3)") {
	Global::State_space<dtime_t>::Workload jobs {
			// Either job 0, 1, or 2 should be first, but job 2 won't be first by default
			Job<dtime_t>{0, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 0, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(10, 17), Interval<dtime_t>(8, 8), 50, 1, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(18, 18), Interval<dtime_t>(8, 8), 50, 2, 2, 2},

			// When either job 3, 4, or 5 is executed as first job, jobs 0 to 2 will miss their deadlines
			Job<dtime_t>{3, Interval<dtime_t>(10, 16), Interval<dtime_t>(100, 100), 900, 3, 3, 3},
			Job<dtime_t>{4, Interval<dtime_t>(16, 16), Interval<dtime_t>(100, 100), 900, 4, 4, 4},
			Job<dtime_t>{5, Interval<dtime_t>(16, 16), Interval<dtime_t>(100, 100), 900, 5, 5, 5},
	};

	const auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	const auto solutions = Reconfiguration::apply_graph_strategy<dtime_t>(problem);
	REQUIRE(solutions.size() == 3);

	for (const auto raw_solution : solutions) {
		const auto solution = dynamic_cast<Reconfiguration::Precedence_solution<dtime_t>*>(raw_solution);
		REQUIRE(solution);
		REQUIRE(solution->from.task <= 2);
		REQUIRE(solution->to.task > 2);
	}

	REQUIRE(Reconfiguration::verify_solution(&problem, solutions));
}

TEST_CASE("TODO handle rating of 0") {
	// Both deadlines are met when job 1 runs first, but job 0 always runs first, resulting in a rating of 0...
	Global::State_space<dtime_t>::Workload jobs {
			Job<dtime_t>{0, Interval<dtime_t>(0, 0), Interval<dtime_t>(800, 800), 900, 1, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(1, 1), Interval<dtime_t>(1, 1), 50, 2, 1, 1},
	};

	const auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	rating_graph.generate_dot_file("test_graph_strategy_rating0.dot", problem, std::vector<Reconfiguration::Rating_graph_cut>());
	REQUIRE(rating_graph.nodes[0].get_rating() == 0.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_graph_strategy_rating0.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	// TODO Figure out solution
}
#endif