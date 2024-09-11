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
		JobID job_id{0, 0};
		Time earliest;
		Time latest;

		void print() override {
			std::cout << "Increase the earliest arrival time of the job with ID " << job_id << " from " << earliest <<
				" to the latest arrival time " << latest << "\n";
		}
	};

	template<class Time> struct PessimisticExecutionTimeSolution final : Solution {
		JobID job_id{0, 0};
		Time bestCase;
		Time worstCase;

		void print() override {
			std::cout << "Increase the best-case running time of the job with ID " << job_id << " from " << bestCase <<
				" to the worst-case running time " << worstCase << "\n";
		}
	};

	struct Precedence_solution final : Solution {
		JobID from;
		JobID to;

		Precedence_solution(const JobID from, const JobID to) : from(from), to(to) {}

		void print() override {
			std::cout << "Add a precedence constraint to ensure that " << from << " must finish before " << to << " can start\n";
		}
	};
}

#endif
