#ifndef RECONFIGURATION_ATTACHMENT_H
#define RECONFIGURATION_ATTACHMENT_H
#include "job_sequence.hpp"

namespace NP::Reconfiguration {
	struct Attachment {
		virtual ~Attachment() = default;
	};

	struct Attachment_job_sequence : Attachment {
		Job_sequence job_sequence;
	};

	struct Attachment_failure_search final: Attachment_job_sequence {
		bool has_missed_deadline = false;
	};
}

#endif
