// Following codes are revised from cuckoofilter https://github.com/efficient/cuckoofilter
// d-ary Cuckoo filter is a generalization of Cuckoo filter which saves even more space
#include "d_ary_cuckoofilter.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using d_ary_cuckoofilter::DaryCuckooFilter;

int main(int argc, char** argv) {
    size_t total_items  = 160000;

    // DaryCuckoofilter takes four template parameters:
    // ItemType, bits_per_item, num_candidate_buckets and TableType
    // ItemType is the type of item you want to insert
    // bits_per_item is the number of bits each item is hashed into
    // num_candidate_buckets is hte number of possible location each item can go
    // TableType is the storage of table, SingleTable by default, MockTable and PackedTable are for
    // experimental usage
    DaryCuckooFilter<size_t, 8, 3, d_ary_cuckoofilter::SingleTable> filter(total_items);

    // Insert
    size_t num_inserted = 0;
    for (size_t i = 0; i < total_items; i++, num_inserted++) {
        if (filter.Add(i) != d_ary_cuckoofilter::Ok) {
            break;
        }
    }

    //  Check if previously inserted items are in the filter, expected
    //  true for all items
    for (size_t i = 0; i < num_inserted; i++) {
        assert(filter.Contain(i) == d_ary_cuckoofilter::Ok);
    }

    // Check non-existing items, a few false positives expected
    size_t total_queries = 0;
    size_t false_queries = 0;
    for (size_t i = total_items; i < 2 * total_items; i++) {
        if (filter.Contain(i) == d_ary_cuckoofilter::Ok) {
            false_queries++;
        }
        total_queries++;
    }
    
    // The output gives:
    // the measured false positive rate
    // table type, fingerprint size, table length, table size in bits, number of inserted elements
    // and the space occupancy rate
    std::cout << "False positive rate is "
              << 100.0 * false_queries / total_queries
              << "%\n";
    std::cout << filter.Info() << "\n";

    // Test deletion
    for (size_t i = 0; i < num_inserted; i++) {
        filter.Delete(i);
    }
    for (size_t i = 0; i < num_inserted; i++) {
        assert(filter.Contain(i) == d_ary_cuckoofilter::NotFound);
    }


    return 0;
 }
