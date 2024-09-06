#ifndef RECONFIGURATION_ATTACHMENT_H
#define RECONFIGURATION_ATTACHMENT_H
#include <jobs.hpp>
#include <vector>

namespace NP::Reconfiguration {
	struct Attachment {
		virtual ~Attachment() = default;
	};

	struct Attachment_job_sequence final: Attachment {
		std::vector<JobID> chosen_job_ids;
	};
}

#endif
