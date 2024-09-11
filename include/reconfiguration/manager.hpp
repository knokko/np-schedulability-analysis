#ifndef RECONFIGURATION_MANAGER_H
#define RECONFIGURATION_MANAGER_H

#include "global/space.hpp"
#include "pessimistic.hpp"
#include "precedence.hpp"

namespace NP::Reconfiguration {
	template<class Time>
	class Manager {
	public:
		static int run_with_automatic_reconfiguration(Scheduling_problem<Time> problem) {
			Analysis_options test_options;
			test_options.timeout = 0;
			test_options.max_depth = 0;
			test_options.early_exit = true; // TODO Change to false when early_exit is supported
			test_options.be_naive = false;
			test_options.use_supernodes = false;

			auto history_agent = Agent_simple_failure_search<Time>();
			auto base_analysis = Global::State_space<Time>::explore(
				problem, test_options, &history_agent
			);

			if (base_analysis->is_schedulable()) {
				std::cout << "The given problem is already schedulable.\n";
				return 0;
			}

			if (history_agent.failures.size() == 0) {
				std::cout << "Not schedulable, but no missed deadlines?\n";
				return -1;
			}

			if constexpr (false) {
				PessimisticReconfigurator<Time> pessimistic_reconfigurator(problem, history_agent.failures, &test_options);
				auto pessimistic_solution = pessimistic_reconfigurator.find_local_minimal_solution();
				if (pessimistic_solution.size() > 0) {
					std::cout << "The given problem is not schedulable, but you can make it schedulable by following these steps:\n";
					for (auto solution : pessimistic_solution) solution->print();
					return 0;
				}
			}

			PrecedenceReconfigurator<Time> precedence_reconfigurator(problem, history_agent.failures, &test_options);
			auto precedence_solution = precedence_reconfigurator.find_local_minimal_solution();
			if (precedence_solution.size() > 0) {
				std::cout << "The given problem is not schedulable, but you can make it schedulable by following these steps:\n";
				for (const auto solution : precedence_solution) solution->print();
				return 0;
			}

			std::cout << "The given problem is not schedulable, and I couldn't find a solution to fix it.\n";
			return 1;
		}
	};
}

#endif
