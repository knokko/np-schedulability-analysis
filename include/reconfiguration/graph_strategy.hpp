#ifndef RECONFIGURATION_GRAPH_STRATEGY_H
#define RECONFIGURATION_GRAPH_STRATEGY_H

#include "global/space.hpp"
#include "attempt_generator.hpp"
#include "cut_explore.hpp"
#include "cut_check.hpp"
#include "cut_test.hpp"
#include "graph_cutter.hpp"
#include "rating_graph.hpp"
#include "solution.hpp"

namespace NP::Reconfiguration {

	static bool compare_cut_backward(const Rating_graph_cut& cut1, const Rating_graph_cut& cut2) {
		return cut1.previous_jobs->length() > cut2.previous_jobs->length();
	}

	template<class Time> std::vector<Solution<Time>*> apply_graph_strategy(const Scheduling_problem<Time> &original_problem) {
		auto problem = original_problem;
		Rating_graph rating_graph;
		Agent_rating_graph<Time>::generate(problem, rating_graph);

		auto cuts = cut_rating_graph(rating_graph);
		std::sort(cuts.begin(), cuts.end(), compare_cut_backward);

		std::vector<Solution<Time>*> selected_solutions;

		while (!cuts.empty()) {
			auto candidate_cut = cuts[cuts.size() - 1];
			cuts.pop_back();

			Agent_cut_explore<Time>::explore_forbidden_jobs(problem, &candidate_cut);
			if (Agent_cut_check<Time>::was_cut_performed(problem, candidate_cut) == 0) continue;

			auto attempts = generate_precedence_attempts<Time>(candidate_cut);

			size_t old_solutions = selected_solutions.size();
			for (const auto &attempt : attempts) {
				attempt->attempt(problem);
				if (Agent_cut_check<Time>::was_cut_performed(problem, candidate_cut) == 0) {
					auto test_result = Agent_cut_test<Time>::perform(problem, cuts);
					if (!test_result.has_unexpected_failures) {
						attempt->add_solutions(selected_solutions, problem);
						break;
					}
				}

				attempt->undo(problem);
			}

			if (selected_solutions.size() == old_solutions) {
				std::cout << "Nothing worked\n";
				selected_solutions.clear();
				return selected_solutions;
			}
		}

		return selected_solutions;
	}
}

#endif