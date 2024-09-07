#ifndef RECONFIGURATION_MANAGER_H
#define RECONFIGURATION_MANAGER_H

#include "global/space.hpp"

namespace NP {
	template<class Time>
	class Reconfiguration_manager {
	public:
		static int run_with_automatic_reconfiguration(Scheduling_problem<Time> problem) {
			Analysis_options test_options;
			test_options.timeout = 0;
			test_options.max_depth = 0;
			test_options.early_exit = false;
			test_options.be_naive = false;
			test_options.use_supernodes = false;

			auto base_analysis = Global::State_space<Time>::explore(
				problem, test_options, new Reconfiguration_agent_job_sequence_history<Time>()
			);
			return 0;
		}
	};
}

#endif
