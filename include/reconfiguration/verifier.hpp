#ifndef RECONFIGURATION_VERIFIER_H
#define RECONFIGURATION_VERIFIER_H

#include "global/space.hpp"
#include "solution.hpp"

namespace NP::Reconfiguration {
	template<class Time> bool verify_solution(const Scheduling_problem<Time> *problem, const std::vector<Solution<Time>*> &solution) {
		Scheduling_problem<Time> fixed_problem = *problem;
		for (const auto modification : solution) {
			modification->apply(fixed_problem);
		}
		validate_prec_cstrnts(fixed_problem.prec, fixed_problem.jobs);

		Analysis_options test_options;
		test_options.use_supernodes = false;

		return Global::State_space<Time>::explore(fixed_problem, test_options)->is_schedulable();
	}
}

#endif
