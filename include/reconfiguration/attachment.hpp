#ifndef RECONFIGURATION_ATTACHMENT_H
#define RECONFIGURATION_ATTACHMENT_H
#include <vector>

namespace NP {
	class Reconfiguration_node_attachment {};

	class Reconfiguration_node_job_sequence_attachment final : Reconfiguration_node_attachment {
		std::vector<size_t> chosen_job_indices;
	};
}

#endif
