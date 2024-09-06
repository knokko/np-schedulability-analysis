#ifndef RECONFIGURATION_AGENT_H
#define RECONFIGURATION_AGENT_H
#include "attachment.hpp"

namespace NP {
	class Reconfiguration_agent {
	public:
		virtual ~Reconfiguration_agent() = default;

	private:
		virtual Reconfiguration_node_attachment* create_initial_node_attachment();
	};

	class Reconfiguration_agent_job_sequence_history final : Reconfiguration_agent {

		Reconfiguration_node_attachment* create_initial_node_attachment() override {
			return new Reconfiguration_node_job_sequence_attachment();
		}
	};
}

#endif
