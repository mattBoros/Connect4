#ifndef CONNECT4_TTABLE_H
#define CONNECT4_TTABLE_H


#include <vector>
#include "StoredSearch.h"
#include "StoredResult.h"


// https://en.wikipedia.org/wiki/Zobrist_hashing
const uint32_t HASH_POSITIONS[2][6][7] = {
        {
                {16812129, 20820876, 4302407,  15893695, 33399393, 2953926,  15993611},
                {4823333, 26590753, 8674078,  11877494, 28173204, 15932950, 31087039},
                {11756281, 33317074, 4194054,  20042485, 17432141, 11328143, 13649657},
                {1298295, 31712701, 268131,   26440987, 29473893, 24812381, 3396855},
                {16533432, 11017114, 22322699, 4551802,  19835366, 25274422, 8878543},
                {1682458,  3218151,  22460647, 16004832, 30119771, 15521672, 32114611}
        },
        {
                {25290570, 32904147, 33022532, 18025101, 23429252, 16767634, 16309043},
                {8371150, 20156821, 10014063, 15865237, 21722530, 26241260, 28536188},
                {22009418, 22695681, 26981527, 28388463, 2300698,  23637305, 27496137},
                {7580332, 13612254, 15672538, 31251397, 4498228,  4988123,  21861809},
                {19893677, 29815725, 15528206, 24450785, 26531555, 20185623, 15220623},
                {22489044, 20619790, 17875156, 2930763,  1792931,  32079375, 5732765}
        }
};

static const uint64_t TTABLE_BUCKET_SIZE = 33554432ULL; // 2^25

class TTableEntry {
public:
    mutable StoredSearch key = NULL_CACHE_KEY;
    mutable StoredResult result = NULL_SEARCH_RESULT;

    inline TTableEntry() {
    }

//    inline TTableEntry(const StoredSearch key, const StoredResult result)
//    : key(key), result(result)
//    {
//    }

    inline TTableEntry(const TTableEntry &other) {
        key = other.key;
        result = other.result;
    }

};

class TTable {
private:
//    mutable CacheEntryFirst* table;
    mutable TTableEntry table[TTABLE_BUCKET_SIZE];
//    mutable uint32_t _size = 0;
//    mutable uint32_t num_unused_entries = 0;

public:
    inline TTable();

    inline void clear() __attribute__ ((hot));

    inline void print_stats() const;

    inline uint32_t size() const;

    inline StoredResult find(const uint32_t index, const StoredSearch key) const __attribute__ ((hot));

    inline void put(const uint32_t index, const StoredSearch key, const StoredResult result);
};

inline TTable::TTable() {
    cout << "initializing ttable" << endl;
}


inline void TTable::clear() {

}

inline uint32_t TTable::size() const {
    return 0;
}

inline void TTable::print_stats() const {

}

inline StoredResult TTable::find(const uint32_t index, const StoredSearch key) const {
//    cout << "calling find" << endl;
    if (table[index].key == key) {
        return table[index].result;
    } else {
        return NULL_SEARCH_RESULT;
    }
}

inline void TTable::put(const uint32_t index, const StoredSearch key, const StoredResult result) {
//    cout << "calling put" << endl;
    table[index].key = key;
    table[index].result = result;
//    table[index] = TTableEntry(key, result);
}


#endif //CONNECT4_TTABLE_H
