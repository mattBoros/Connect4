#ifndef CONNECT4_CACHE_H
#define CONNECT4_CACHE_H


#include <vector>
#include "StoredSearch.h"
#include "StoredResult.h"


static const uint64_t CACHE_BUCKET_SIZE = 33554432ULL; // 2^25

static const uint32_t CACHE_CAPACITY = 20000000;

class CacheEntry {
public:
    const StoredSearch key;
    const StoredResult result;
    mutable uint8_t num_uses = 0;

    mutable CacheEntry *next = nullptr;

    inline CacheEntry(const StoredSearch key,
                      const StoredResult result);

    inline ~CacheEntry() __attribute__ ((hot, no_stack_limit));

    inline uint32_t size() const;

};

inline CacheEntry::CacheEntry(const StoredSearch key,
                  const StoredResult result)
        : key(key), result(result) {
}

inline CacheEntry::~CacheEntry() {
    delete next;
    next = nullptr;
}

inline uint32_t CacheEntry::size() const {
    if(next == nullptr){
        return 1;
    } else {
        return 1 + next->size();
    }
}


class CacheEntryFirst {
public:
    mutable CacheEntry *entry = nullptr;

    inline ~CacheEntryFirst();

    inline void clear_all() __attribute__ ((hot));

    inline void clear_unused() __attribute__ ((hot));

    inline void prepend(const StoredSearch key, const StoredResult result);

    inline uint32_t size() const;
};


inline CacheEntryFirst::~CacheEntryFirst()  {
    clear_all();
}

inline void CacheEntryFirst::clear_all() {
    delete entry;
    entry = nullptr;
}

inline void CacheEntryFirst::clear_unused() {
    while(entry != nullptr && entry->num_uses == 0){
        const CacheEntry* to_delete = entry;
        entry = to_delete->next;
        to_delete->next = nullptr;
        delete to_delete;
    }
    if(entry == nullptr){
        return;
    }
    // now the first entry is not null, and num_uses >= 1
    const CacheEntry *previous = entry;
    const CacheEntry *current = entry->next;
    while(current != nullptr){
        if(current->num_uses == 0){
            previous->next = current->next;
            current->next = nullptr;
            delete current;
            if(previous->next == nullptr){
                break;
            }
            current = previous->next;
        }
        previous = current;
        current = current->next;
    }
}

inline void CacheEntryFirst::prepend(const StoredSearch key, const StoredResult result) {
    CacheEntry* new_beginning = new CacheEntry(key, result);
//        CacheEntry* old_beginning = entry;
//        entry = new_beginning;
//        new_beginning->next = old_beginning;
    new_beginning->next = entry;
    entry = new_beginning;
}

inline uint32_t CacheEntryFirst::size() const {
    if(entry == nullptr){
        return 0;
    }
    return entry->size();
}


class Cache {
private:
//    mutable CacheEntryFirst* table;
    mutable CacheEntryFirst table[CACHE_BUCKET_SIZE];
    mutable uint32_t _size = 0;
    mutable uint32_t num_unused_entries = 0;

public:
    inline Cache();

    inline void print_stats();

    inline void clear_all() __attribute__ ((hot));

    inline void clear_unused() __attribute__ ((hot));

    inline void clear() __attribute__ ((hot));

    inline uint32_t size() const;

    inline StoredResult find(const uint32_t index, const StoredSearch key) const __attribute__ ((hot));

    inline void put(const uint32_t index, const StoredSearch key, const StoredResult result);
};

inline Cache::Cache() {
//        table = new CacheEntryFirst[CACHE_BUCKET_SIZE];
    cout << "initializing ttable" << endl;
    clear();
    cout << "done initializing ttable" << endl;
}

inline void Cache::print_stats() {
//        uint32_t num_cached_with_0_uses = 0;
//        uint64_t sum_nonzero_uses = 0;
//        uint32_t num_nonzero_uses = 0;
//        uint16_t max_num_uses = 0;
//        uint32_t num_uses_counts[100] = {0};
//        for (uint32_t bucket = 0; bucket < CACHE_BUCKET_SIZE; ++bucket) {
//            CacheEntry* entry = table[bucket].entry;
//            while(entry != nullptr){
//                ++num_uses_counts[entry->num_uses];
//                max_num_uses = entry->num_uses > max_num_uses ? entry->num_uses : max_num_uses;
//                if(entry->num_uses == 0){
//                    num_cached_with_0_uses++;
//                } else {
//                    sum_nonzero_uses += entry->num_uses;
//                    ++num_nonzero_uses;
//                }
//                entry = entry->next;
//            }
//        }
//
//        cout << "num cached with 0 uses : " << num_cached_with_0_uses << endl;
//        cout << "average non-zero uses  : " << sum_nonzero_uses * 1.0 / num_nonzero_uses << endl;
//        cout << "max num uses           : " << max_num_uses << endl;
//
//        for (int i = 0; i < max_num_uses+1; ++i) {
//            cout << "num of uses=" << i << " : " << num_uses_counts[i] << endl;
//        }

//        uint32_t num_collisions = 0;
//        uint64_t collision_sum = 0;
//        uint16_t max_bucket_size = 0;
//        for (uint32_t bucket = 0; bucket < CACHE_BUCKET_SIZE; ++bucket) {
//            uint16_t bucket_size = table[bucket].size();
//            if(bucket_size >= 2){
//                num_collisions++;
//                collision_sum += bucket_size;
//            }
//            if(bucket_size > max_bucket_size) {
//                max_bucket_size = bucket_size;
//            }
//        }
//        int *bucket_size_counts = new int[max_bucket_size];
//        for (uint32_t bucket = 0; bucket < CACHE_BUCKET_SIZE; ++bucket) {
//            uint16_t bucket_size = table[bucket].size();
//            bucket_size_counts[bucket_size-1]++;
//        }
//        cout << "# of collisions : " << (int) num_collisions << endl;
//        cout << "Avg bucket size when there's a collision : " << collision_sum * 1.0 / num_collisions << endl;
//        cout << "Max bucket size : " << (int) max_bucket_size << endl;
//        for (int bucket_size = 1; bucket_size < max_bucket_size+1; ++bucket_size) {
//            cout << "number of buckets of size " << bucket_size << " : " << bucket_size_counts[bucket_size-1] << endl;
//        }
//        cout << "about to delete" << endl;
//        delete[] bucket_size_counts;
//        cout << "done deleting" << endl;
}

inline void Cache::clear_all() {
    for (uint32_t i = 0; i < CACHE_BUCKET_SIZE; ++i) {
        table[i].clear_all();
    }
//        delete[] table;
//        table = new CacheEntryFirst[CACHE_BUCKET_SIZE];
    _size = 0;
    num_unused_entries = 0;
}

inline void Cache::clear_unused() {
    for (uint32_t i = 0; i < CACHE_BUCKET_SIZE; ++i) {
        table[i].clear_unused();
    }
    _size = _size - num_unused_entries;
    num_unused_entries = 0;
}

inline void Cache::clear() {
    cout << "num unused entries : " << num_unused_entries << endl;
    if(num_unused_entries <= 3.0 * CACHE_CAPACITY / 8.0){
        cout << "cleared all" << endl;
        clear_all();
    } else {
        cout << "cleared unused" << endl;
        clear_unused();
    }
}

inline uint32_t Cache::size() const {
    return _size;
}

inline StoredResult Cache::find(const uint32_t index, const StoredSearch key) const {
    if(table[index].entry == nullptr){
        return NULL_SEARCH_RESULT;
    }
    const CacheEntry* current = table[index].entry;
    while(true){
        if(current->key == key){
            if(current->num_uses == 0){
                --num_unused_entries;
            }
            ++current->num_uses;
            return current->result;
        }
        if(current->next == nullptr){
            break;
        }
        current = current->next;
    }

    return NULL_SEARCH_RESULT;
}

inline void Cache::put(const uint32_t index, const StoredSearch key, const StoredResult result) {
    ++_size;
    ++num_unused_entries;
    table[index].prepend(key, result);
}


#endif //CONNECT4_CACHE_H
