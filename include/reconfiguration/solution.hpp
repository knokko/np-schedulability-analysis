#ifndef RECONFIGURATION_SOLUTION_H
#define RECONFIGURATION_SOLUTION_H

namespace NP::Reconfiguration {

	struct Solution {
		virtual ~Solution() = default;

		virtual void print() {
			assert(false);
			std::cout << "To be implemented!\n";
		}
	};

	template<class Time> struct PessimisticArrivalTimeSolution final : Solution {
		unsigned long job_id{};
		Time earliest;
		Time latest;

		void print() override {
			std::cout << "Increase the earliest arrival time of the job with ID " << job_id << " from " << earliest <<
				" to the latest arrival time " << latest << "\n";
		}
	};

	template<class Time> struct PessimisticExecutionTimeSolution final : Solution {
		unsigned long job_id{};
		Time bestCase;
		Time worstCase;

		void print() override {
			std::cout << "Increase the best-case running time of the job with ID " << job_id << " from " << bestCase <<
				" to the worst-case running time " << worstCase << "\n";
		}
	};
}

#endif
