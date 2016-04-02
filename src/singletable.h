// SingleTable is the hashtable of d-ary Cuckoo filter
#ifndef _SINGLE_TABLE_H_
#define _SINGLE_TABLE_H_

#include <sstream>
#include <xmmintrin.h>
#include <assert.h>

#include "bitsutil.h"
#include "debug.h"


namespace d_ary_cuckoofilter {
    
    // the most naive table implementation: one huge bit array
    template <size_t bits_per_tag> //8,16,32
    class SingleTable {
        
        static const size_t bytes_per_bucket = (bits_per_tag + 7) >> 3;
        
        struct Bucket {
            unsigned char bits_[bytes_per_bucket];
        } __attribute__((__packed__));
        
        size_t num_buckets;
        
        // using a pointer adds one more indirection
        Bucket *buckets_;
        
    public:
        static const uint32_t TAGMASK = (1ULL << bits_per_tag) - 1; //mask
        
        explicit
        SingleTable(size_t num_candidate_buckets, size_t max_num_keys) {
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
        
        ~SingleTable() {
            delete [] buckets_;
        }
        
        void CleanupTags() { memset(buckets_, 0, bytes_per_bucket * num_buckets); }
        
        size_t SizeInBytes() const { return bytes_per_bucket * num_buckets; }
        
        size_t SizeInBuckets() const { return num_buckets; }
        
        size_t HashTableSize() const { return num_buckets; }
        
        std::string Info() const  {
            std::stringstream ss;
            ss << "\t\tSingleHashtable with tag size: " << bits_per_tag << " bits \n";
            ss << "\t\tTotal rows: " << num_buckets << "\n";
            ss << "\t\tTable size in bits: " << SizeInBuckets() * bits_per_tag << "\n";
            return ss.str();
        }
        
        
        inline uint32_t ReadTag(const size_t i) const {
            const unsigned char *p = buckets_[i].bits_;
            uint32_t tag;
            /* following code only works for little-endian */
            if (bits_per_tag == 8) {
                tag = *((uint8_t*) p);
            }
            else if (bits_per_tag == 16) {
                tag = *((uint16_t*) p);
            }
            else if (bits_per_tag == 32) {
                tag = *((uint32_t*) p);
            }
            return tag & TAGMASK;
        }
        
        
        inline void  WriteTag(const size_t i, const uint32_t t) {
            unsigned char *p = buckets_[i].bits_;
            uint32_t tag = t & TAGMASK;
            /* following code only works for little-endian */
            if (bits_per_tag == 8) {
                ((uint8_t*) p)[0] =  tag;
                //printFingerprint(((uint8_t*) p)[0], tag);
                
            }
            else if (bits_per_tag == 16) {
                ((uint16_t*) p)[0] = tag;
            }
            else if (bits_per_tag == 32) {
                ((uint32_t*) p)[0] = tag;
            }
            
            return;
        }
        
        inline bool  FindTagInBucket(const size_t i,  const uint32_t tag) const {
            // caution: unaligned access & assuming little endian
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
        
    };// SingleTable
}

#endif // #ifndef _SINGLE_TABLE_H_
