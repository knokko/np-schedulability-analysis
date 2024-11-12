#ifndef CONFIG_PARALLEL
#include "doctest.h"
#undef NDEBUG

#include "global/space.hpp"
#include "reconfiguration/rating_graph.hpp"
#include "reconfiguration/graph_cutter.hpp"
#include "reconfiguration/cut_explore.hpp"

using namespace NP;

TEST_CASE("cut_explorer") {
	Global::State_space<dtime_t>::Workload jobs {
			// Job 0 has considerable execution time variant, which allows lots of jobs to be potentially second
			Job<dtime_t>{0, Interval<dtime_t>(0, 0), Interval<dtime_t>(10, 20), 20, 1, 0, 0},

			// Either job 1, 2, or 3 should be next, but the fourth job won't be next my default
			Job<dtime_t>{1, Interval<dtime_t>(18, 18), Interval<dtime_t>(3, 3), 30, 2, 1, 1},
			Job<dtime_t>{2, Interval<dtime_t>(17, 17), Interval<dtime_t>(3, 3), 30, 3, 2, 2},
			Job<dtime_t>{3, Interval<dtime_t>(18, 18), Interval<dtime_t>(3, 3), 30, 4, 3, 3},

			// All subsequent jobs must be executed after jobs 1, 2, and 3 are finished
			Job<dtime_t>{4, Interval<dtime_t>(13, 13), Interval<dtime_t>(100, 100), 900, 5, 4, 4},
			Job<dtime_t>{5, Interval<dtime_t>(14, 14), Interval<dtime_t>(100, 100), 900, 6, 5, 5},
			Job<dtime_t>{6, Interval<dtime_t>(15, 15), Interval<dtime_t>(100, 100), 900, 7, 6, 6},
	};

	auto problem = Scheduling_problem<dtime_t>(jobs, std::vector<Precedence_constraint<dtime_t>>());

	Reconfiguration::Rating_graph rating_graph;
	Reconfiguration::Agent_rating_graph<dtime_t>::generate(problem, rating_graph);
	REQUIRE(rating_graph.nodes[0].rating < 1.0);

	auto cuts = Reconfiguration::cut_rating_graph(rating_graph);
	rating_graph.generate_dot_file("test_cut_explore.dot", problem, cuts);

	REQUIRE(cuts.size() == 1);
	auto &cut = cuts[0];
	REQUIRE(cut.allowed_jobs.size() == 2);
	REQUIRE(cut.forbidden_jobs.size() == 1);
	Reconfiguration::Agent_cut_explore<dtime_t>::explore_fully(problem, &cut);

	REQUIRE(cut.extra_allowed_jobs.size() == 1);
	REQUIRE(cut.extra_allowed_jobs[0] == 3);

	REQUIRE(cut.extra_forbidden_jobs.size() == 2);
	auto extra0 = cut.extra_forbidden_jobs[0];
	REQUIRE((extra0 == 5 || extra0 == 6));
	auto extra1 = cut.extra_forbidden_jobs[1];
	REQUIRE((extra1 == 5 || extra1 == 6));
}
#endif
