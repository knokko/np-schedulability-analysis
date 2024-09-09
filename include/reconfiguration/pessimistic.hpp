#ifndef RECONFIGURATION_PESSIMISTIC_H
#define RECONFIGURATION_PESSIMISTIC_H

#include <unordered_set>

namespace NP::Reconfiguration {
	template<class Time> class PessimisticReconfigurator {
	private:
		std::pmr::unordered_set<JobID> critical_job_ids;
	};
}

#endif
