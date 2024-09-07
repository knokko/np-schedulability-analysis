#ifndef RECONFIGURATION_ATTACHMENT_H
#define RECONFIGURATION_ATTACHMENT_H
#include <vector>

namespace NP {
	struct Reconfiguration_attachment {
		virtual ~Reconfiguration_attachment() = default;
	};

	struct Reconfiguration_attachment_job_sequence final: Reconfiguration_attachment {
		std::vector<unsigned long> chosen_job_ids;
	};
}

#endif
