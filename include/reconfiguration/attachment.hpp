#ifndef RECONFIGURATION_ATTACHMENT_H
#define RECONFIGURATION_ATTACHMENT_H
#include <jobs.hpp>
#include <vector>
#include "index_collection.hpp"

namespace NP::Reconfiguration {
	struct Attachment {
		virtual ~Attachment() = default;
	};

	struct Attachment_job_sequence : Attachment {
		std::vector<JobID> chosen_job_ids;
	};

	struct Attachment_failure_search final: Attachment_job_sequence {
		bool has_missed_deadline = false;
	};
}

#endif
