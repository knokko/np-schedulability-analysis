using namespace NP;

Global::State_space<dtime_t>::Workload jobs1a{
		// high-frequency task
		Job<dtime_t>{1, Interval<dtime_t>( 0,  0), Interval<dtime_t>(1, 2), 10, 10, 0, 0},
		Job<dtime_t>{2, Interval<dtime_t>(10, 10), Interval<dtime_t>(1, 2), 20, 20, 1, 1},
		Job<dtime_t>{3, Interval<dtime_t>(20, 20), Interval<dtime_t>(1, 2), 30, 30, 2, 2},
		Job<dtime_t>{4, Interval<dtime_t>(30, 30), Interval<dtime_t>(1, 2), 40, 40, 3, 3},
		Job<dtime_t>{5, Interval<dtime_t>(40, 40), Interval<dtime_t>(1, 2), 50, 50, 4, 4},
		Job<dtime_t>{6, Interval<dtime_t>(50, 50), Interval<dtime_t>(1, 2), 60, 60, 5, 5},

		// middle task
		Job<dtime_t>{7, Interval<dtime_t>( 0,  0), Interval<dtime_t>(7, 8), 30, 30, 6, 6},
		Job<dtime_t>{8, Interval<dtime_t>(30, 30), Interval<dtime_t>(7, 8), 60, 60, 7, 7},

		// the long task
		Job<dtime_t>{9, Interval<dtime_t>( 0,  0), Interval<dtime_t>(3, 13), 60, 60, 8, 8}
};