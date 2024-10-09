#include "doctest.h"
#include "reconfiguration/index_collection.hpp"
#include "jobs.hpp"

using namespace NP;

TEST_CASE("Index_collection insert to vector") {
    Job_index large_number = 50000000000000;

    NP::Reconfiguration::Index_collection indices;
    indices.insert(1);
    indices.insert(2);
    indices.insert(large_number);

    CHECK(!indices.contains(0));
    CHECK(indices.contains(1));
    CHECK(indices.contains(2));
    CHECK(!indices.contains(3));

    CHECK(indices.contains(large_number));
    CHECK(!indices.contains(large_number - 1));
    CHECK(!indices.contains(large_number + 1));

	// TODO Test iterator
}

TEST_CASE("Index_collection insert to set") {
    Job_index large_number = 50000;

    NP::Reconfiguration::Index_collection indices;
    indices.insert(1);
    indices.insert(2);
    indices.insert(large_number);

    for (Job_index index = 1000; index < 40000; index++) indices.insert(index);

    CHECK(!indices.contains(0));
    CHECK(indices.contains(1));
    CHECK(indices.contains(2));
    CHECK(!indices.contains(3));

    CHECK(indices.contains(large_number));
    CHECK(!indices.contains(large_number - 1));
    CHECK(!indices.contains(large_number + 1));

    CHECK(!indices.contains(999));
    CHECK(indices.contains(1000));
    CHECK(indices.contains(1001));
    CHECK(indices.contains(39998));
    CHECK(indices.contains(39999));
    CHECK(!indices.contains(40000));
}

TEST_CASE("Index_collection merge") {
    Job_index large_number = 50000000000000;

    NP::Reconfiguration::Index_collection index_set;
    for (Job_index index = 2; index < 5; index++) index_set.insert(index);

    NP::Reconfiguration::Index_collection index_vector;
    index_vector.insert(0);
    index_vector.insert(large_number);

    NP::Reconfiguration::Index_collection merged_into_set;
    merged_into_set.merge(&index_set);
    merged_into_set.merge(&index_vector);

    NP::Reconfiguration::Index_collection merged_into_vector;
    merged_into_vector.merge(&index_vector);
    merged_into_vector.merge(&index_set);

    index_set.merge(&index_vector);

    NP::Reconfiguration::Index_collection* allIndices[] = { &merged_into_set, &merged_into_vector, &index_set };
    for (const auto &indices : allIndices) {
        CHECK(indices->contains(0));
        CHECK(!indices->contains(1));
        CHECK(indices->contains(2));
        CHECK(indices->contains(3));
        CHECK(indices->contains(4));
        CHECK(!indices->contains(5));

        CHECK(indices->contains(large_number));
        CHECK(!indices->contains(large_number - 1));
        CHECK(!indices->contains(large_number + 1));
    }
}
