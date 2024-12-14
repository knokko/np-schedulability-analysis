#ifndef RECONFIGURATION_ATTEMPT_GENERATOR_H
#define RECONFIGURATION_ATTEMPT_GENERATOR_H

#include "attempt.hpp"
#include "graph_cutter.hpp"

namespace NP::Reconfiguration {

	template<class Time> std::vector<std::unique_ptr<Attempt<Time>>> generate_precedence_attempts(const Rating_graph_cut &cut) {
		assert(!cut.allowed_jobs.empty());
		assert(!cut.forbidden_jobs.empty());

		std::vector<std::unique_ptr<Attempt<Time>>> attempts;
		for (const auto &allowed : cut.allowed_jobs) {
			auto forbidden_jobs = cut.forbidden_jobs;
			forbidden_jobs.reserve(cut.extra_forbidden_jobs.size());
			for (const auto forbidden : cut.extra_forbidden_jobs) forbidden_jobs.push_back(forbidden);
			attempts.push_back(std::make_unique<Precedence_attempt<Time>>(allowed, forbidden_jobs));
		}

		return attempts;
	}
}

#endif
