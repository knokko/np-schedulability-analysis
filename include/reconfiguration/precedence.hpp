#ifndef RECONFIGURATION_PRECEDENCE_H
#define RECONFIGURATION_PRECEDENCE_H

#include "agent.hpp"

namespace NP::Reconfiguration {
	template<class Time> class PrecedenceReconfigurator {
		Scheduling_problem<Time> *original_problem;
		Scheduling_problem<Time> adapted_problem;
		Analysis_options *test_options;
		std::vector<FailedSequence> forbidden_edges;

		void find_all_problematic_edges() {
			Agent_exhaustive_failure_search<Time> agent;
			agent.failures = forbidden_edges;

			while (true) {
				auto result = Global::State_space<Time>::explore(
					adapted_problem, *test_options, &agent
				);
				if (result->is_schedulable()) {
					this->forbidden_edges = agent.failures;
					std::cout << "Forbidden edges are:\n";
					for (auto edge : this->forbidden_edges) edge.print();
					return;
				}

				if (agent.instant_failure) {
					std::cout << "Hopeless\n";
					return;
				}
			}
		}

	public:
		PrecedenceReconfigurator(
			Scheduling_problem<Time> &problem,
			std::vector<FailedSequence> failures,
			Analysis_options *test_options
		) : original_problem(&problem), adapted_problem(problem), test_options(test_options), forbidden_edges(std::move(failures)) {}

		std::vector<Solution*> find_local_minimal_solution() {
			find_all_problematic_edges();
			auto agent = Agent_failure_alternative_search<Time>(forbidden_edges);
			auto result = Global::State_space<Time>::explore(
                adapted_problem, *test_options, &agent
            );
			std::cout << "alternatives are:\n";
			for (const auto &alternatives : agent.alternatives) {
				for (const auto job : alternatives) std::cout << job << ", ";
				std::cout << "\n";
			}

			std::vector<Solution*> solutions;

			for (int index = 0; index < forbidden_edges.size(); index++) {
				const auto &forbidden_edge = forbidden_edges[index];
				const auto forbidden_job = forbidden_edge.chosen_job_ids[forbidden_edge.chosen_job_ids.size() - 1];
				for (const auto alternative_job : agent.alternatives[index]) {
					adapted_problem.prec.push_back(Precedence_constraint<Time>(alternative_job, forbidden_job, Interval<Time>(0, 0)));
					validate_prec_cstrnts(adapted_problem.prec, adapted_problem.jobs);
					auto adapted_result = Global::State_space<Time>::explore(
                        adapted_problem, *test_options, &agent
                    );
					if (adapted_result->is_schedulable()) {
						solutions.push_back(new Precedence_solution(alternative_job, forbidden_job));
						break;
					}
					adapted_problem.prec.pop_back();
				}

				if (solutions.size() != index + 1) return {};
			}
			return solutions;
		}
	};
}

#endif
