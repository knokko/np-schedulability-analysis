#ifndef RECONFIGURATION_SOLUTION_H
#define RECONFIGURATION_SOLUTION_H

#include "global/space.hpp"

namespace NP::Reconfiguration {

	template<class Time> struct Solution {
		virtual ~Solution() = default;

		virtual void print() {
			assert(false);
			std::cout << "To be implemented!\n";
		}

		virtual void apply(Scheduling_problem<Time> &problem) {
			assert(false);
			std::cout << "To be implemented!\n";
		}
	};

	template<class Time> struct PessimisticArrivalTimeSolution final : Solution<Time> {
		JobID job_id{0, 0};
		Time earliest;
		Time latest;

		void print() override {
			std::cout << "Increase the earliest arrival time of the job with ID " << job_id << " from " << earliest <<
				" to the latest arrival time " << latest << "\n";
		}
	};

	template<class Time> struct PessimisticExecutionTimeSolution final : Solution<Time> {
		JobID job_id{0, 0};
		Time bestCase;
		Time worstCase;

		void print() override {
			std::cout << "Increase the best-case running time of the job with ID " << job_id << " from " << bestCase <<
				" to the worst-case running time " << worstCase << "\n";
		}
	};

	template<class Time> struct Precedence_solution final : Solution<Time> {
		JobID from;
		JobID to;

		Precedence_solution(const JobID from, const JobID to) : from(from), to(to) {}

		void print() override {
			std::cout << "Add a precedence constraint to ensure that " << from << " must finish before " << to << " can start\n";
		}

		void apply(Scheduling_problem<Time> &problem) override {
			problem.prec.push_back(Precedence_constraint<Time>(from, to, Interval<Time>()));
		}
	};
}

#endif
