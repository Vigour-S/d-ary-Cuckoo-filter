// MockTable is a simple implementation for the purpose of measuring the impact of fingerprint size in
// loadfactor of d-ary Cuckoo filter
// The expected relationship between fingerprint size f and number of elements n is f=Omega(logn)
#ifndef _MOCK_TABLE_H_
#define _MOCK_TABLE_H_

#include <sstream>
#include <xmmintrin.h>
#include <assert.h>

#include "bitsutil.h"
#include "debug.h"


namespace d_ary_cuckoofilter {
    
    template <size_t bits_per_tag> //any size is OK
    class MockTable {
        
        struct Bucket {
            uint32_t bits_;
        };
        
        size_t num_buckets;
        
        Bucket *buckets_;
        
    public:
        static const uint32_t TAGMASK = (1ULL << bits_per_tag) - 1;
        
        explicit
        MockTable(size_t num_candidate_buckets, size_t max_num_keys) {
            switch (num_candidate_buckets) {
                case 2:
                    num_buckets = upperpower2(max_num_keys);
                    break;
                case 3:
                    num_buckets = upperpower3(max_num_keys);
                    break;
                case 4:
                    num_buckets = upperpower4(max_num_keys);
                    break;
                case 5:
                    num_buckets = upperpower5(max_num_keys);
                    break;
                default:
                    break;
            }
            double frac = (double) max_num_keys / num_buckets;
            switch (num_candidate_buckets) {
                case 2:
                    if (frac > 0.42) num_buckets <<= 1; break;
                case 3:
                    if (frac > 0.91) num_buckets *= 3;  break;
                case 4:
                    if (frac > 0.97) num_buckets *= 4;  break;
                case 5:
                    if (frac > 0.985) num_buckets *= 5; break;
            }
            buckets_ = new Bucket[num_buckets];
            CleanupTags();
        }
        
        ~MockTable() {
            delete [] buckets_;
        }
        
        void CleanupTags() { memset(buckets_, 0,  num_buckets); }
        
        size_t SizeInBits() const { return bits_per_tag * num_buckets; }
        
        size_t SizeInBuckets() const { return num_buckets; }
        
        size_t HashTableSize() const { return num_buckets; }
        
        
        std::string Info() const  {
            std::stringstream ss;
            ss << "\t\tMockHashTable with tag size: " << bits_per_tag << " bits \n";
            ss << "\t\tTotal rows: " << num_buckets << "\n";
            ss << "\t\tTable size in bits: " << SizeInBuckets() * bits_per_tag << "\n";
            return ss.str();
        }
        
        
        inline uint32_t ReadTag(const size_t i) const {
            return buckets_[i].bits_ & TAGMASK;
        }
        
        
        inline void  WriteTag(const size_t i, const uint32_t t) {
            buckets_[i].bits_ = t & TAGMASK;
            return;
        }
        
        inline bool  FindTagInBucket(const size_t i,  const uint32_t tag) const {
            if (ReadTag(i) == tag)
                return true;
            return false;
        }// FindTagInBucket
        
        inline  bool  DeleteTagFromBucket(const size_t i,  const uint32_t tag) {
            if (ReadTag(i) == tag) {
                assert (FindTagInBucket(i, tag) == true);
                WriteTag(i, 0);
                return true;
            }
            return false;
        }// DeleteTagFromBucket
        
        inline  bool  InsertTagToBucket(const size_t i,  const uint32_t tag,
                                        const bool kickout, uint32_t& oldtag) {
            if (ReadTag(i) == 0) {
                WriteTag(i, tag);
                return true;
            }
            if (kickout) {
                oldtag = ReadTag(i);
                WriteTag(i, tag);
            }
            return false;
        }// InsertTagToBucket
        
    };// MockTable
}

#endif
