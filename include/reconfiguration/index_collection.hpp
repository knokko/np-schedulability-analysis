#ifndef RECONFIGURATION_JOB_SUBSET_H
#define RECONFIGURATION_JOB_SUBSET_H
#include "index_set.hpp"
#include "jobs.hpp"
#include <vector>
#include <ntdef.h>

namespace NP::Reconfiguration {
    class Index_collection {
        Index_set set;
        std::vector<Job_index> vector;
        Job_index largest_index;

        const bool uses_set() {
            return set.get_vector_size() > 0;
        }
    public:
        const bool contains(Job_index job_index) {
            if (uses_set()) return set.contains(job_index);
            else return std::find(vector.begin(), vector.end(), job_index) != vector.end();
        }

        void insert(Job_index new_index) {
            largest_index = std::max(largest_index, new_index);
            auto potential_set_size = (largest_index / 64) * 8;
            auto potential_vector_size = (vector.size() + 1) * sizeof(Job_index);
            bool should_use_set = potential_set_size <= 10 * potential_vector_size;

            // Convert from vector to set
            if (should_use_set && !uses_set()) {
                set.add(largest_index);
                for (const auto job_index : vector) set.add(job_index);
                vector.clear();
                vector.shrink_to_fit();
            }

            // Convert from set to vector
            if (!should_use_set && uses_set()) {
                for (size_t int_index = 0; int_index < set.get_vector_size(); int_index++) {
                    for (int bit_index = 0; bit_index < 64; bit_index++) {
                        auto job_index = bit_index + 64 * int_index;
                        if (set.contains(job_index)) vector.push_back(job_index);
                    }
                }
                set.clear();
            }

            if (uses_set()) set.add(new_index);
            else if (!contains(new_index)) vector.push_back(new_index);
        }

        void merge(Index_collection &other) {
            largest_index = std::max(this->largest_index, other.largest_index);
            this->insert(largest_index);
            if (other.uses_set()) {
                for (size_t int_index = 0; int_index < other.set.get_vector_size(); int_index++) {
                    for (int bit_index = 0; bit_index < 64; bit_index++) {
                        auto job_index = bit_index + 64 * int_index;
                        if (other.set.contains(job_index)) this->insert(job_index);
                    }
                }
            } else {
                for (const auto job_index : other.vector) {
                    this->insert(job_index);
                }
            }
        }
    };
}

#endif
