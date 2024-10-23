#ifndef RECONFIGURATION_ATTEMPT_GENERATOR_H
#define RECONFIGURATION_ATTEMPT_GENERATOR_H

#include "attempt.hpp"
#include "graph_cutter.hpp"

namespace NP::Reconfiguration {

	template<class Time> std::vector<std::unique_ptr<Attempt<Time>>> generate_precedence_attempts(Rating_graph_cut cut) {
		assert(!cut.allowed_jobs.empty());
		assert(!cut.forbidden_jobs.empty());

		std::vector<std::unique_ptr<Attempt<Time>>> attempts;
		for (const auto &allowed : cut.allowed_jobs) {
			attempts.push_back(std::make_unique<Precedence_attempt<Time>>(allowed, cut.forbidden_jobs));
		}

		return attempts;
	}
}

#endif
