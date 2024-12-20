#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/rating_graph.hpp"
#include "reconfiguration/graph_cutter.hpp"
#include "reconfiguration/cut_explore.hpp"

using namespace NP;

TEST_CASE("cut_explorer: mini first job choice") {
	// Without intervention, either job 0 or job 2 will run first.
	// - when job 0 is first, all jobs will meet their deadlines
	// - when the exploration agent forbids job 0, it becomes possible that job 1 goes first, which is also fine
	// - when job 2 is first, the other jobs will miss their deadlines
	Global::State_space<dtime_t>::Workload jobs {
			Job<dtime_t>{0, Interval<dtime_t>(0, 1), Interval<dtime_t>(1, 1), 5, 0, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(1, 1), Interval<dtime_t>(1, 1), 5, 1, 1, 1},

			Job<dtime_t>{2, Interval<dtime_t>(0, 1), Interval<dtime_t>(10, 10), 20, 2, 2, 2},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	std::cout << "test1\n";
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	std::cout << "test2\n";
	REQUIRE(rating_graph.nodes[0].rating > 0.0);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	std::cout << "test3\n";
	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	std::cout << "test4\n";
	rating_graph.generate_dot_file("test_cut_explore_mini1.dot", problem, cuts);
	std::cout << "test5\n";

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 1);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_forbidden_jobs(problem, &cut);

	//REQUIRE(cut.extra_allowed_jobs.size() == 1); TODO Maybe bring this back later
	//REQUIRE(cut.extra_allowed_jobs[0] == 1);

	REQUIRE(cut.extra_forbidden_jobs.size() == 0);
}

TEST_CASE("cut_explorer: small first job choice") {
	Global::State_space<dtime_t>::Workload jobs {
			// When either job 0 or 1 goes first, no deadlines will be missed.
			// Note that job 1 can only go first if the exploration agent forbids the other two jobs.
			Job<dtime_t>{0, Interval<dtime_t>(0, 2), Interval<dtime_t>(2, 2), 10, 0, 0, 0},
			Job<dtime_t>{1, Interval<dtime_t>(2, 2), Interval<dtime_t>(1, 1), 10, 1, 1, 1},

			// When job 2 goes first, jobs 0 and 1 will miss their deadlines
			Job<dtime_t>{2, Interval<dtime_t>(0, 1), Interval<dtime_t>(100, 100), 200, 2, 2, 2},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	std::cout << "start rating graph\n";
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	std::cout << "finished rating graph\n";
	rating_graph.generate_dot_file("test_cut_explore_mini2.dot", problem, std::vector<Reconfiguration::Rating_graph_cut>());
	REQUIRE(rating_graph.nodes[0].rating > 0.0);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_cut_explore_mini2_cut.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 1);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_forbidden_jobs(problem, &cut);

	//REQUIRE(cut.extra_allowed_jobs.size() == 1); TODO Maybe bring back later
	//REQUIRE(cut.extra_allowed_jobs[0] == 1);

	REQUIRE(cut.extra_forbidden_jobs.size() == 0);
}

TEST_CASE("cut_explorer: explore first job choices") {
	Global::State_space<dtime_t>::Workload jobs {
			// Either job 1, 2, or 3 should be first, but job 3 won't be first by default
			Job<dtime_t>{1, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 2, 0, 1},
			Job<dtime_t>{2, Interval<dtime_t>(10, 17), Interval<dtime_t>(8, 8), 50, 3, 1, 2},
			Job<dtime_t>{3, Interval<dtime_t>(18, 18), Interval<dtime_t>(8, 8), 50, 4, 2, 3},

			// When either job 4, 5, or 6 is executed as first job, jobs 1 to 3 will miss their deadlines
			Job<dtime_t>{4, Interval<dtime_t>(10, 17), Interval<dtime_t>(100, 100), 900, 5, 3, 4},
			Job<dtime_t>{5, Interval<dtime_t>(17, 17), Interval<dtime_t>(100, 100), 900, 6, 4, 5},
			Job<dtime_t>{6, Interval<dtime_t>(17, 17), Interval<dtime_t>(100, 100), 900, 7, 5, 6},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_cut_explore1.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 2);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_forbidden_jobs(problem, &cut);

	//REQUIRE(cut.extra_allowed_jobs.size() == 1);
	//REQUIRE(cut.extra_allowed_jobs[0] == 2); TODO Maybe bring back later

//	REQUIRE(cut.extra_forbidden_jobs.size() == 2);
//	auto extra0 = cut.extra_forbidden_jobs[0];
//	REQUIRE((extra0 == 4 || extra0 == 5));
//	auto extra1 = cut.extra_forbidden_jobs[1]; TODO Maybe bring back later
//	REQUIRE((extra1 == 4 || extra1 == 5));
}

TEST_CASE("cut_explorer: explore first job choices complex") {
	Global::State_space<dtime_t>::Workload jobs {
			// Either job 1, 2, or 3 should be first, but job 3 won't be first by default
			Job<dtime_t>{1, Interval<dtime_t>(10, 18), Interval<dtime_t>(8, 8), 50, 2, 0, 1},
			Job<dtime_t>{2, Interval<dtime_t>(10, 17), Interval<dtime_t>(8, 8), 50, 3, 1, 2},
			Job<dtime_t>{3, Interval<dtime_t>(18, 18), Interval<dtime_t>(8, 8), 50, 4, 2, 3},

			// When either job 4, 5, or 6 is executed as first job, jobs 1 to 3 will miss their deadlines
			Job<dtime_t>{4, Interval<dtime_t>(10, 16), Interval<dtime_t>(100, 100), 900, 5, 3, 4},
			Job<dtime_t>{5, Interval<dtime_t>(16, 16), Interval<dtime_t>(100, 100), 900, 6, 4, 5},
			Job<dtime_t>{6, Interval<dtime_t>(16, 16), Interval<dtime_t>(100, 100), 900, 7, 5, 6},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_cut_explore1_complex.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 2);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_forbidden_jobs(problem, &cut);

	//REQUIRE(cut.extra_allowed_jobs.size() == 1);
	//REQUIRE(cut.extra_allowed_jobs[0] == 2); TODO Maybe bring back later

	REQUIRE(cut.extra_forbidden_jobs.size() == 2);
	auto extra0 = cut.extra_forbidden_jobs[0];
	REQUIRE((extra0 == 4 || extra0 == 5));
	auto extra1 = cut.extra_forbidden_jobs[1];
	REQUIRE((extra1 == 4 || extra1 == 5));
}

TEST_CASE("cut_explorer: explore second job choices") {
	Global::State_space<dtime_t>::Workload jobs {
			// Job 0 has considerable execution time variation, which allows lots of jobs to be potentially second
			Job<dtime_t>{0, Interval<dtime_t>(0, 0), Interval<dtime_t>(10, 20), 20, 1, 0, 0},

			// Either job 1, 2, or 3 should be next, but job 3 won't be next by default
			Job<dtime_t>{1, Interval<dtime_t>(18, 18), Interval<dtime_t>(3, 3), 30, 2, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(17, 17), Interval<dtime_t>(3, 3), 30, 3, 2, 2},
			Job<dtime_t>{3, Interval<dtime_t>(18, 18), Interval<dtime_t>(3, 3), 30, 4, 3, 3},

			// When either job 4, 5, or 6 is executed as second job, jobs 1 to 3 will miss their deadlines
			Job<dtime_t>{4, Interval<dtime_t>(13, 13), Interval<dtime_t>(100, 100), 900, 5, 4, 4},
			Job<dtime_t>{5, Interval<dtime_t>(14, 14), Interval<dtime_t>(100, 100), 900, 6, 5, 5},
			Job<dtime_t>{6, Interval<dtime_t>(15, 15), Interval<dtime_t>(100, 100), 900, 7, 6, 6},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_cut_explore2.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 2);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_forbidden_jobs(problem, &cut);

//	REQUIRE(cut.extra_allowed_jobs.size() == 1); TODO Maybe bring back later
//	REQUIRE(cut.extra_allowed_jobs[0] == 3);

	REQUIRE(cut.extra_forbidden_jobs.size() == 2);
	auto extra0 = cut.extra_forbidden_jobs[0];
	REQUIRE((extra0 == 5 || extra0 == 6));
	auto extra1 = cut.extra_forbidden_jobs[1];
	REQUIRE((extra1 == 5 || extra1 == 6));
}
#endif