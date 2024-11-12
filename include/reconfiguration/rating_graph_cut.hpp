#ifndef RATING_GRAPH_CUT_H
#define RATING_GRAPH_CUT_H

#include <vector>
#include "sub_graph.hpp"

namespace NP::Reconfiguration {
	struct Rating_graph_cut {
		std::shared_ptr<Sub_graph> previous_jobs;
		std::vector<Job_index> forbidden_jobs;
		std::vector<Job_index> allowed_jobs;
		std::vector<Job_index> extra_forbidden_jobs;
		std::vector<Job_index> extra_allowed_jobs;
	};
}

#endif