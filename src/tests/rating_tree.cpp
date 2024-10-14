#include "doctest.h"

#include "global/space.hpp"
#include "reconfiguration/rating_tree.hpp"

using namespace NP;

TEST_CASE("Rating tree") {
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

	Reconfiguration::Rating_tree rating_tree;
	Reconfiguration::Agent_rating_tree<dtime_t>::generate(problem, rating_tree);

	CHECK(rating_tree.nodes.size() == 11);

	// Node 0 is the root, and can only take job 0
	CHECK(rating_tree.nodes[0].rating == 0.5f);
	CHECK(rating_tree.nodes[0].children.size() == 1);
	CHECK(rating_tree.nodes[0].children[0].taken_job == 0);
	CHECK(rating_tree.nodes[0].children[0].child_index == 1);

	// Node 1 can only take job 6
	CHECK(rating_tree.nodes[1].rating == 0.5f);
	CHECK(rating_tree.nodes[1].children.size() == 1);
	CHECK(rating_tree.nodes[1].children[0].taken_job == 6);
	CHECK(rating_tree.nodes[1].children[0].child_index == 2);

	// Node 2 can take either job 1 or job 8, where job 8 is a poor choice
	CHECK(rating_tree.nodes[2].rating == 0.5f);
	CHECK(rating_tree.nodes[2].children.size() == 2);
	int failed_node_index = rating_tree.nodes[2].children[0].child_index;
	int node_index3 = rating_tree.nodes[2].children[1].child_index;
	if (rating_tree.nodes[2].children[1].taken_job == 8) {
		CHECK(rating_tree.nodes[2].children[0].taken_job == 1);
		failed_node_index = node_index3;
	} else {
		CHECK(rating_tree.nodes[2].children[1].taken_job == 1);
		CHECK(rating_tree.nodes[2].children[0].taken_job == 8);
	}

	CHECK(rating_tree.nodes[failed_node_index].rating == 0.0);
	CHECK(rating_tree.nodes[failed_node_index].children.size() == 0);

	for (int index = 3; index < rating_tree.nodes.size(); index++) {
		if (index == failed_node_index) continue;
		CHECK(rating_tree.nodes[index].rating == 1.0);

		if (index != rating_tree.nodes.size() - 1) {
			CHECK(rating_tree.nodes[index].children.size() == 1);
			CHECK(rating_tree.nodes[index].children[0].child_index > index);
		} else CHECK(rating_tree.nodes[index].children.size() == 0);
	}
}
