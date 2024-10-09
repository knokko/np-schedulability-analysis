#ifndef RECONFIGURATION_JOB_SEQUENCE_H
#define RECONFIGURATION_JOB_SEQUENCE_H
#include "index_set.hpp"
#include "jobs.hpp"
#include "index_collection.hpp"
#include <vector>

namespace NP::Reconfiguration {

	class Job_sequence {
		std::vector<std::unique_ptr<Index_collection>> choices;

	public:
		void merge(const Job_sequence &other) {
			assert(other.choices.size() == this->choices.size());

			for (size_t index = 0; index < this->choices.size(); index++) {
				this->choices[index]->merge(other.choices[index].get());
			}
		}

		void add_next(Job_index next_job) {
			this->choices.push_back(std::make_unique<Index_collection>());
			this->choices[this->choices.size() - 1]->insert(next_job);
		}

		Job_sequence extended_copy(Job_index next_job) const {
			Job_sequence extended;
			extended.choices.reserve(this->choices.size() + 1);

			for (size_t index = 0; index < this->choices.size(); index++) {
				auto copied_collection = std::make_unique<Index_collection>();
				copied_collection->merge(this->choices[index].get());
				extended.choices.push_back(std::move(copied_collection));
			}

			auto last_collection = std::make_unique<Index_collection>();
			last_collection->insert(next_job);
			extended.choices.push_back(std::move(last_collection));

			return extended;
		}

		bool is_prefix_of(const Job_sequence &other, bool strict) const {
			if (strict && other.choices.size() <= this->choices.size()) return false;
			if (other.choices.size() < this->choices.size()) return false;

			for (int index = 0; index < choices.size(); index++) {
				if (!this->choices[index]->is_subset_of(other.choices[index].get())) return false;
			}

			return true;
		}

		Index_collection* last() const {
			return this->choices[this->choices.size() - 1].get();
		}

		Index_collection* get(size_t index) const {
			return this->choices[index].get();
		}

		size_t length() const {
			return this->choices.size();
		}
	};
}

#endif
