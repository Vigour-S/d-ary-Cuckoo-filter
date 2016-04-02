// PackedTable is a simple implementation to test if d-ary Cuckoo filter with additional bits can handle
// dataset of any size with a high loadfactor
#ifndef _PACKED_TABLE_H_
#define _PACKED_TABLE_H_

#include <sstream>
#include <xmmintrin.h>
#include <assert.h>

#include "bitsutil.h"
#include "debug.h"



namespace d_ary_cuckoofilter {
    
    // the most naive table implementation: one huge bit array
    template <size_t bits_per_tag> //any size is OK
    class PackedTable {
        
        struct Bucket {
            uint32_t bits_;
            uint32_t mark;
        };
        
        size_t num_buckets;
        size_t mocktablesize;
        size_t num_candidate_buckets;
        
        // using a pointer adds one more indirection
        Bucket *buckets_;
        
    public:
        static const uint32_t TAGMASK = (1ULL << bits_per_tag) - 1;
        
        explicit
        PackedTable(size_t num, size_t max_num_keys) {
            
            num_candidate_buckets = num;
            
            switch (num_candidate_buckets) {
                case 2:
                    mocktablesize = upperpower2(max_num_keys);
                    break;
                case 3:
                    mocktablesize = upperpower3(max_num_keys);
                    break;
                case 4:
                    mocktablesize = upperpower4(max_num_keys);
                    break;
                case 5:
                    mocktablesize = upperpower5(max_num_keys);
                    break;
                default:
                    std::cout << "the valid candidate bucket num is 2~5";
            }
            switch (num_candidate_buckets) {
                case 2:
                    num_buckets = ceil(max_num_keys / 0.42); break;
                case 3:
                    num_buckets = ceil(max_num_keys / 0.91); break;
                case 4:
                    num_buckets = ceil(max_num_keys / 0.96); break;
                case 5:
                    num_buckets = ceil(max_num_keys / 0.985); break;
            }
            buckets_ = new Bucket[num_buckets];
            CleanupTags();
        }
        
        ~PackedTable() {
            delete [] buckets_;
        }
        
        void CleanupTags() { memset(buckets_, 0,  num_buckets); }
        
        size_t SizeInBuckets() const { return num_buckets; }
        
        size_t HashTableSize() const { return mocktablesize; }
        
        std::string Info() const  {
            std::stringstream ss;
            ss << "\t\tPackedHashTable with tag size: " << bits_per_tag << " bits \n";
            ss << "\t\tPackedHashTable with mark size: " << markbits(num_candidate_buckets) << " bits \n";
            ss << "\t\tTotal rows: " << num_buckets << "\n";
            ss << "\t\tTable size in bits: " << SizeInBuckets() * (bits_per_tag + markbits(num_candidate_buckets)) << "\n";
            return ss.str();
        }
        
        
        inline uint32_t ReadTag(const size_t i) const {
            return buckets_[i % num_buckets].bits_ & TAGMASK;
        }
        
        inline uint32_t ReadMark(const size_t i) const {
            return buckets_[i % num_buckets].mark;
        }
        
        
        inline void  WriteTag(const size_t i, const uint32_t t) {
            buckets_[i % num_buckets].bits_ = t & TAGMASK;
            buckets_[i % num_buckets].mark = i / num_buckets;
            return;
        }
        
        inline bool  FindTagInBucket(const size_t i,  const uint32_t tag) const {
            if (ReadTag(i) == tag && ReadMark(i) == (i/num_buckets))
                return true;
            return false;
        }// FindTagInBucket
        
        inline  bool  DeleteTagFromBucket(const size_t i,  const uint32_t tag) {
            if (FindTagInBucket(i, tag)) {
                WriteTag(i, 0);
                return true;
            }
            return false;
        }// DeleteTagFromBucket
        
        inline  bool  InsertTagToBucket(size_t&  i,  const uint32_t tag,
                                        const bool kickout, uint32_t& oldtag) {
            if (ReadTag(i) == 0) {
                WriteTag(i, tag);
                return true;
            }
            if (kickout) {
                size_t mark = ReadMark(i);
                oldtag = ReadTag(i);
                WriteTag(i, tag);
                i = i%num_buckets + mark * num_buckets;
            }
            return false;
        }// InsertTagToBucket
        
    };// MockTable
}

#endif // #ifndef _MOCK_TABLE_H_
