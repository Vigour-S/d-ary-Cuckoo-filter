#ifndef _CUCKOO_FILTER_H_
#define _CUCKOO_FILTER_H_

#include "debug.h"
#include "hashutil.h"
#include "singletable.h"
#include "mocktable.h"
#include "packedtable.h"

#include <stdlib.h>
#include <cassert>
#include <typeinfo>

namespace d_ary_cuckoofilter {
    // status returned
    enum Status {
        Ok = 0,
        NotFound = 1,
        NotEnoughSpace = 2,
        NotSupported = 3,
    };
    
    // maximum number of cuckoo kicks before claiming failure
    const size_t kMaxCuckooCount = 5000;
    
    // DaryCuckooFilter provides methods of Add, Delete, Contain.
    // DaryCuckoofilter takes four template parameters:
    // ItemType, bits_per_item, num_candidate_buckets and TableType
    // ItemType is the type of item you want to insert
    // bits_per_item is the number of bits each item is hashed into
    // num_candidate_buckets is hte number of possible location each item can go
    // TableType is the storage of table, SingleTable by default, MockTable and PackedTable are for
    // experimental usage
    template <typename ItemType,
    size_t bits_per_item,
    size_t num_candidate_buckets,
    template<size_t> class TableType>
    class DaryCuckooFilter {
        // Storage of items
        TableType<bits_per_item> *table_;
        
        // Number of items stored
        size_t  num_items_;
        
        typedef struct {
            size_t index;
            uint32_t tag;
            bool used;
        } VictimCache;
        
        VictimCache victim_;
        
        inline size_t IndexHash(uint32_t hv) const {
            return hv % table_->HashTableSize();
        }
        
        inline uint32_t TagHash(uint32_t hv) const {
            uint32_t tag;
            tag = hv & ((1ULL << bits_per_item) - 1);
            tag += (tag == 0);
            return tag;
        }
        
        inline void GenerateIndexTagHash(const ItemType &item,
                                         size_t* index,
                                         uint32_t* tag) const {
            
            std::string hashed_key = HashUtil::SHA1Hash((const char*) &item,
                                                        sizeof(item));
            uint64_t hv = *((uint64_t*) hashed_key.c_str());
            
            *index = IndexHash((uint32_t) (hv >> 32));
            *tag   = TagHash((uint32_t) (hv & 0xFFFFFFFF));
        }
        
        inline size_t AltIndex(const size_t index, const uint32_t tag) const {
            switch (num_candidate_buckets) {
                case 2: return index ^ IndexHash(HashUtil::BobHash((const void*) (&tag), 4));
                case 3: return xor_(IndexHash(HashUtil::BobHash((const void*) (&tag), 4)) , index, 3);
                case 4: return xor_(IndexHash(HashUtil::BobHash((const void*) (&tag), 4)) , index, 4);
                case 5: return xor_(IndexHash(HashUtil::BobHash((const void*) (&tag), 4)) , index, 5);
            }
            
        }
        
        Status AddImpl(const size_t i, const uint32_t tag);
        
        // load factor is the fraction of occupancy
    public:
        double LoadFactor() const {
            return 1.0 * Size()  / table_->SizeInBuckets();
        }
        
        double BitsPerItem() const {
            return 8.0 * table_->SizeInBytes() / Size();
        }
        
        size_t SizeInBits() const {
            return table_->SizeInBuckets() * bits_per_item;
        }
        
    public:
        explicit DaryCuckooFilter(const size_t max_num_keys): num_items_(0) {
            
            victim_.used = false;
            table_  = new TableType<bits_per_item>(num_candidate_buckets, max_num_keys);
        }
        
        ~DaryCuckooFilter() {
            delete table_;
        }
        
        
        // Add an item to the filter.
        Status Add(const ItemType& item);
        
        // Report if the item is inserted, with false positive rate.
        Status Contain(const ItemType& item) const;
        
        // Delete an key from the filter
        Status Delete(const ItemType& item);
        
        /* methods for providing stats  */
        // summary infomation
        std::string Info() const;
        
        // number of current inserted items;
        size_t Size() const { return num_items_; }
        
        // size of the filter in bytes.
        size_t SizeInBytes() const { return table_->SizeInBytes(); }
    };
    
    
    template <typename ItemType, size_t bits_per_item, size_t num_candidate_buckets,
    template<size_t> class TableType>
    Status
    DaryCuckooFilter<ItemType, bits_per_item, num_candidate_buckets, TableType>::Add(const ItemType& item) {
        size_t i;
        uint32_t tag;
        
        if (victim_.used) {
            return NotEnoughSpace;
        }
        
        GenerateIndexTagHash(item, &i, &tag);
        return AddImpl(i, tag);
    }
    
    template <typename ItemType, size_t bits_per_item, size_t num_candidate_buckets,
    template<size_t> class TableType>
    Status
    DaryCuckooFilter<ItemType, bits_per_item, num_candidate_buckets, TableType>::AddImpl(const size_t i, const uint32_t tag) {
        uint32_t curtag = tag;
        uint32_t oldtag = 0;
        size_t index[5]; //index[0] for curindex, index[1~4] for altindex
        index[0] = i;
        
        srand((unsigned)time(NULL));
        
        for (size_t j =1; j<num_candidate_buckets; j++) {
            index[j] = AltIndex(index[j-1], curtag);
        }
        assert(index[0] == AltIndex(index[num_candidate_buckets-1], curtag));
        
        bool kickout = false;
        
        for (size_t j=0; j<num_candidate_buckets; j++) {
            if (table_->InsertTagToBucket(index[j], curtag, kickout, oldtag)) {
                num_items_++;
                return Ok;
            }
        }
        
        // we use ramdom walk strategy
        switch (rand() % num_candidate_buckets) {
            case 0: break;
            case 1: index[0] = index[1]; break;
            case 2: index[0] = index[2]; break;
            case 3: index[0] = index[3]; break;
            case 4: index[0] = index[4]; break;
        }

        for (uint32_t count = 0; count < kMaxCuckooCount; count++) {
            bool kickout = true;
            oldtag = 0;
            table_->InsertTagToBucket(index[0], curtag, kickout, oldtag);
            curtag = oldtag;
            
            for (size_t j=1; j<num_candidate_buckets; j++) {
                index[j] = AltIndex(index[j-1], curtag);
            }
            
            switch (rand() % (num_candidate_buckets-1)) {
                case 0: index[0] = index[1]; break;
                case 1: index[0] = index[2]; break;
                case 2: index[0] = index[3]; break;
                case 3: index[0] = index[4]; break;
            }
            
            if (table_->InsertTagToBucket(index[0], curtag, false, oldtag)) {
                num_items_++;
                return Ok;
            }
        }

        std::cout << "Not Enough Space" << std::endl;
        victim_.index = index[0];
        victim_.tag = curtag;
        victim_.used = true;
        return Ok;
    }//AddImpl
    
    template <typename ItemType,
    size_t bits_per_item,
    size_t num_candidate_buckets,
    template<size_t> class TableType>
    Status
    DaryCuckooFilter<ItemType, bits_per_item, num_candidate_buckets, TableType>::Contain(const ItemType& key) const {
        bool found = false;
        size_t index[5];
        uint32_t tag;
        
        GenerateIndexTagHash(key, index, &tag);
        for (size_t j =1; j<num_candidate_buckets; j++) {
            index[j] = AltIndex(index[j-1], tag);
        }
        assert(index[0] == AltIndex(index[num_candidate_buckets-1], tag));
        
        for (size_t j =0; j<num_candidate_buckets; j++) {
            found = found || (index[j] == victim_.index);
        }
        found = found && victim_.used && (tag == victim_.tag);
        
        for (size_t j =0; j<num_candidate_buckets; j++) {
            if (found || table_->FindTagInBucket(index[j], tag)) {
                return Ok;
            }
        }
        
        return NotFound;
    }
    
    template <typename ItemType,
    size_t bits_per_item,
    size_t num_candidate_buckets,
    template<size_t> class TableType>
    Status
    DaryCuckooFilter<ItemType, bits_per_item, num_candidate_buckets, TableType>::Delete(const ItemType& key) {
        size_t index[5];
        uint32_t tag;
        bool victim = false;
        
        GenerateIndexTagHash(key, index, &tag);
        for (size_t j =1; j<num_candidate_buckets; j++) {
            index[j] = AltIndex(index[j-1], tag);
        }
        assert(index[0] == AltIndex(index[num_candidate_buckets-1], tag));
        
        for (size_t j =0; j<num_candidate_buckets; j++) {
            if (table_->DeleteTagFromBucket(index[j], tag)) {
                num_items_--;
                goto TryEliminateVictim;
            }
        }
        
        for (size_t j =0; j<num_candidate_buckets; j++) {
            victim = victim || (index[j] == victim_.index);
        }
        
        if (victim_.used && tag == victim_.tag && victim) {
            victim_.used = false;
            return Ok;
        } else {
            return NotFound;
        }
        
    TryEliminateVictim:
        if (victim_.used) {
            victim_.used = false;
            size_t i = victim_.index;
            uint32_t tag = victim_.tag;
            AddImpl(i, tag);
        }
        return Ok;
    }
    
    template <typename ItemType,
    size_t bits_per_item,
    size_t num_candidate_buckets,
    template<size_t> class TableType>
    std::string DaryCuckooFilter<ItemType, bits_per_item, num_candidate_buckets, TableType>::Info() const {
        std::stringstream ss;
        ss << "DaryCuckooFilter Status:\n"
        << table_->Info()
        << "\t\tKeys stored: " << Size() << "\n"
        << "\t\tLoad factor: " << LoadFactor() << "%\n";
        return ss.str();
    }
}  // namespace d_ary_cuckoofilter

#endif
