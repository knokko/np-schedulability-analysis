#ifndef RECONFIGURATION_JOB_SUBSET_H
#define RECONFIGURATION_JOB_SUBSET_H
#include "index_set.hpp"
#include "jobs.hpp"
#include <vector>
#include <ntdef.h>
#include <iterator>
#include <cstddef>

namespace NP::Reconfiguration {
    class Index_collection {
        Index_set set;
        std::vector<Job_index> vector;
        Job_index largest_index;

        bool uses_set() const {
            return set.get_vector_size() > 0;
        }
    public:

        bool contains(Job_index job_index) const {
            if (uses_set()) return set.contains(job_index);
            else return std::find(vector.begin(), vector.end(), job_index) != vector.end();
        }

        void insert(Job_index new_index) {
            auto new_largest_index = std::max(largest_index, new_index);
            auto potential_set_size = (new_largest_index / 64) * 8;
            auto potential_vector_size = (vector.size() + 1) * sizeof(Job_index);
            bool should_use_set = potential_set_size <= 10 * potential_vector_size;

            // Convert from vector to set
            if (should_use_set && !uses_set()) {
                set.add(new_largest_index);
                for (const auto job_index : vector) set.add(job_index);
                vector.clear();
            }

            // Convert from set to vector
            if (!should_use_set && uses_set()) {
				for (const auto job_index : *this) vector.push_back(job_index);
                set.clear();
            }

            if (uses_set()) set.add(new_index);
            else if (!contains(new_index)) vector.push_back(new_index);
			largest_index = new_largest_index;
        }

        void merge(Index_collection *other) {
            largest_index = std::max(this->largest_index, other->largest_index);
            this->insert(largest_index);
            for (const auto job_index : *other) this->insert(job_index);
        }

		bool is_subset_of(Index_collection *other) const {
			for (const auto job_index : *this) {
				if (!other->contains(job_index)) return false;
			}
			return true;
		}

		struct Iterator {
			using iterator_category = std::input_iterator_tag;
			using value_type = Job_index;

			Iterator(
					Index_collection const* collection, size_t index
			) : collection(collection), index(index) {
				advance();
			}

			Iterator& operator++(){
				index++;
				advance();
				return *this;
			}

			bool operator==(const Iterator& other) const {
				return this->collection == other.collection && this->index == other.index;
			}

			const Job_index& operator*() const {
				if (collection->uses_set()) return index;
				else return collection->vector[index];
			}
		private:
			Index_collection const* collection;
			size_t index;

			void advance() {
				if (!collection->uses_set()) return;

				while (index <= collection->largest_index && !collection->set.contains(index)) index++;
			}
		};

		Iterator begin() const {
			return Iterator(this, 0);
		}

		Iterator end() const {
			if (uses_set()) return Iterator(this, largest_index + 1);
			else return Iterator(this, vector.size());
		}
    };
}

#endif
