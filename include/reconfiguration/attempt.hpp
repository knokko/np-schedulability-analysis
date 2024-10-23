#ifndef RECONFIGURATION_ATTEMPT_H
#define RECONFIGURATION_ATTEMPT_H

#include "solution.hpp"

namespace NP::Reconfiguration {

	template<class Time> struct Attempt {
		virtual ~Attempt() = default;

		virtual void print() {
			assert(false);
		}

		virtual void add_solutions(std::vector<Solution*> &solutions, Scheduling_problem<Time> &problem) {
			assert(false);
		}

		virtual void attempt(Scheduling_problem<Time> &problem) {
			assert(false);
		}

		virtual void undo(Scheduling_problem<Time> &problem) {
			assert(false);
		}
	};

	template<class Time> struct Precedence_attempt final : Attempt<Time> {
		Precedence_attempt(Job_index before, std::vector<Job_index> after) : before(before), after(after) {}

		const Job_index before;
		const std::vector<Job_index> after;

		void print() override {
			std::cout << "Add precedence constraint such that job index " << before << " must finish before starting job indices";
			for (const auto after_index : after) std::cout << " " << after_index;
			std::cout << std::endl;
		}

		void add_solutions(std::vector<Solution*> &solutions, Scheduling_problem<Time> &problem) override {
			const JobID before_id = problem.jobs[before].get_id();
			for (const auto after_index : after) {
				const JobID after_id = problem.jobs[after_index].get_id();
				solutions.push_back(new Precedence_solution(before_id, after_id));
			}
		}

		void attempt(Scheduling_problem<Time> &problem) override {
			const JobID before_id = problem.jobs[before].get_id();
			for (const auto after_index : after) {
				problem.prec.push_back(Precedence_constraint<Time>(before_id, problem.jobs[after_index].get_id(), Interval<Time>(0, 0)));
			}
			validate_prec_cstrnts(problem.prec, problem.jobs);
		}

		void undo(Scheduling_problem<Time> &problem) override {
			problem.prec.erase(std::remove_if(problem.prec.begin(), problem.prec.end(), [this, &problem](const auto& constraint) {
				if (constraint.get_fromID() == problem.jobs[before].get_id()) {
					for (const auto after_index : after) {
						if (constraint.get_toID() == problem.jobs[after_index].get_id()) return true;
					}
				}
				return false;
			}), problem.prec.end());
			validate_prec_cstrnts(problem.prec, problem.jobs);
		}
	};
}

#endif
