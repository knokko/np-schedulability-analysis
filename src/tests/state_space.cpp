#include "doctest.h"

#include <iostream>


#include "schedule_space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();

TEST_CASE("[NP state space] Find all next jobs") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 8), 100, 1},
		Job<dtime_t>{2, I( 7,  7), I(5, 5),  100, 2},
		Job<dtime_t>{3, I(10, 10), I(1, 11),  100, 3},
	};

	SUBCASE("State evolution") {
		Uniproc::Schedule_state<dtime_t> v1;

		Uniproc::Schedule_state<dtime_t> v2{v1, jobs[1], 1, inf, inf};

		CHECK(v2.earliest_finish_time() == 12);
		CHECK(v2.latest_finish_time() == 12);
	}

	SUBCASE("Naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

	SUBCASE("Exploration with merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

}

TEST_CASE("[NP state space] Consider large enough interval") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 3},
		Job<dtime_t>{2, I( 7,  7),  I(5, 5),  100, 2},
		Job<dtime_t>{3, I(10, 10),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 10);

	CHECK(nspace.get_finish_times(jobs[1]).from()  == 12);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 20);

	CHECK(nspace.get_finish_times(jobs[2]).from()  == 15);
	CHECK(nspace.get_finish_times(jobs[2]).until() == 19);

	auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 10);

	CHECK(space.get_finish_times(jobs[1]).from()  == 12);
	CHECK(space.get_finish_times(jobs[1]).until() == 20);

	CHECK(space.get_finish_times(jobs[2]).from()  == 15);
	CHECK(space.get_finish_times(jobs[2]).until() == 19);
}



TEST_CASE("[NP state space] Respect priorities") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 2},
		Job<dtime_t>{2, I( 0,  0),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 15);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  5);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(space.get_finish_times(jobs[0]).until() == 15);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() ==  5);
}

TEST_CASE("[NP state space] Respect jitter") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  1), I(3, 10),  100, 2},
		Job<dtime_t>{2, I( 0,  1),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 16);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 15);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 16);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() == 15);
}

TEST_CASE("[NP state space] Be eager") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		Job<dtime_t>{2, I( 0,  0),  I(1, 20),  100, 3},
		Job<dtime_t>{3, I(10, 10),  I(5,  5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  25);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  30);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(space.get_finish_times(jobs[1]).until() ==  25);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(space.get_finish_times(jobs[2]).until() ==  30);
}


TEST_CASE("[NP state space] Be eager, with short deadline") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		Job<dtime_t>{2, I( 9,  9),  I(1, 15),   25, 3},
		Job<dtime_t>{3, I(30, 30),  I(5,  5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  24);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  35);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(space.get_finish_times(jobs[1]).until() ==  24);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(space.get_finish_times(jobs[2]).until() ==  35);
}


TEST_CASE("[NP state space] Treat equal-priority jobs correctly") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(    0,    10),  I( 2,    50),  2000, 1},
		Job<dtime_t>{2, I(    0,    10),  I(50,  1200),  5000, 2},
		Job<dtime_t>{3, I( 1000,  1010),  I( 2,    50),  3000, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);
}

TEST_CASE("[NP state space] Equal-priority simultaneous arrivals") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(    0,    10),  I(  2,    50),  2000, 1},
		Job<dtime_t>{2, I(    0,    10),  I(100,   150),  2000, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  10 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  10 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);
}

TEST_CASE("[NP state space] don't skip over deadline-missing jobs") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(  100,   100),  I(   2,    50),   200, 1},
		Job<dtime_t>{2, I(    0,     0),  I(1200,  1200),  5000, 2},
		Job<dtime_t>{3, I(  200,   250),  I( 2,    50),    6000, 3},
		Job<dtime_t>{4, I(  200,   250),  I( 2,    50),    6000, 4},
		Job<dtime_t>{5, I(  200,   250),  I( 2,    50),    6000, 5},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(!nspace.is_schedulable());

	CHECK(nspace.number_of_edges() == 2);
	CHECK(nspace.number_of_states() == 3);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(!space.is_schedulable());

	CHECK(space.number_of_edges() == 2);
	CHECK(space.number_of_states() == 3);
}

TEST_CASE("[NP state space] explore across bucket boundaries") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(  100,   100),  I(  50,   50),  10000, 1},
		Job<dtime_t>{2, I( 3000,  3000),  I(4000, 4000),  10000, 2},
		Job<dtime_t>{3, I( 6000,  6000),  I(   2,    2),  10000, 3},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs, 2);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.number_of_edges() == 3);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs, 2);
	CHECK(space.is_schedulable());

	CHECK(space.number_of_edges() == 3);
}

