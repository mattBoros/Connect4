#include <iostream>

using namespace std;

#include <chrono>
#include <sys/time.h>
#include <fstream>

using namespace std::chrono;

#ifndef CONNECT4_TIME_H
#define CONNECT4_TIME_H

namespace TIME {
    static uint64_t positions_searched = 0;
    static uint64_t num_wrong_searches = 0;
    static uint64_t cutoffs = 0;
    static uint64_t non_cutoffs = 0;
    static uint64_t num_max_alpha_zeros = 0;
    static uint64_t num_max_height_ifs = 0;
    static uint64_t num_below_max_height_ifs  = 0;
    static uint64_t cache_hits = 0;
    static uint64_t cache_misses = 0;
    static uint64_t ttable_hits = 0;
    static uint64_t ttable_misses = 0;
    static uint64_t num_last_depths = 0;
    static uint64_t num_fm_zero = 0;
    static uint64_t num_fm_one= 0;
    static uint64_t num_fm_two= 0;
    static uint64_t num_short_circuit_wins = 0;


    static long int isPossibleMove = 0;

    static long int applyTime = 0;
    static long int getActionTime = 0;
    static long int bsSetTime = 0;
    static long int bsGetTime = 0;

    static long int getTime() {
//        return 0;
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return tp.tv_sec * 1000 + tp.tv_usec / 1000;
    }
}


#endif //CONNECT4_TIME_H

