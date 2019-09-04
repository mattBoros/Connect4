
#ifndef CONNECT4_UTIL_H
#define CONNECT4_UTIL_H

#include "AlphaBeta.h"
#include "Time.h"
#include "State.h"

namespace Util {


    static const uint64_t MASKS[6][7] = {
            {1ULL,  256ULL,  65536ULL,   16777216ULL,  4294967296ULL,   1099511627776ULL,  281474976710656ULL}, // row 0
            {2ULL,  512ULL,  131072ULL,  33554432ULL,  8589934592ULL,   2199023255552ULL,  562949953421312ULL}, // row 1
            {4ULL,  1024ULL, 262144ULL,  67108864ULL,  17179869184ULL,  4398046511104ULL,  1125899906842624ULL}, // row 2
            {8ULL,  2048ULL, 524288ULL,  134217728ULL, 34359738368ULL,  8796093022208ULL,  2251799813685248ULL}, // row 3
            {16ULL, 4096ULL, 1048576ULL, 268435456ULL, 68719476736ULL,  17592186044416ULL, 4503599627370496ULL}, // row 4
            {32ULL, 8192ULL, 2097152ULL, 536870912ULL, 137438953472ULL, 35184372088832ULL, 9007199254740992ULL}}; // row 5



    /*
     _______
     _____W_
     _B___B_
     _WW__B_
     WBWWBBW
     WBBBWWB
     */
    static inline State getBoard(const string s) {
        uint64_t bPieces = 0ULL;
        uint64_t wPieces = 0ULL;

        uint8_t col_heights[7] = {0};

        uint8_t numSpotsFilled = 0;

        for (uint8_t i = 0; i < 42; ++i) {
            const uint8_t row = 5 - i / 7;
            const uint8_t col = i % 7;

            if (s[i] != '_') {
                col_heights[col] = max(row, col_heights[col]);
            }

            if (s[i] == 'B') {
                bPieces = bPieces | MASKS[row][col];
                numSpotsFilled++;
            }
            if (s[i] == 'W') {
                wPieces = wPieces | MASKS[row][col];
                numSpotsFilled++;
            }

        }

        return State(
                bPieces,
                wPieces,
                33554432 - 1,
                0, 0, 0, 0, 0, 0, 0
//,0ULL
//                ,numSpotsFilled
        );
    }

    static inline int8_t min(const int8_t x, const int8_t y) {
//        return y ^ ((x ^ y) & -(x < y));
        return y < x ? y : x;
    }

    static const int8_t MAX_MATRIX[3][3] = {
            {-1, 0, 1},
            {0,  0, 1},
            {1,  1, 1}
    };

    static inline int8_t max(const int8_t x, const int8_t y) __attribute__ ((hot, const));

    static inline uint64_t reverse_board(const uint64_t board)  __attribute__ ((hot, const));

    static void print_ull(const uint64_t board) {
        for (uint8_t col = 0; col < 8; ++col) {
            for (uint8_t row = 0; row < 8; ++row) {
                const uint8_t i = col * 8 + row;
                const uint64_t mask = (1ULL << i);
                bool is_filled = board & mask;
                cout << (int) is_filled;
            }
            cout << endl;
        }
        cout << endl;
    }

}

static inline int8_t Util::max(const int8_t x, const int8_t y) {
    return MAX_MATRIX[x + 1][y + 1];
//        cout << "max(" << (int) x << ", " << (int) y << ")" << endl;
//        return x ^ ((x ^ y) & -(x < y));
//        return y > x ? y : x;
//        const bool comp = y > x;
//        return comp*y + (!comp)*x;
}

static inline uint64_t Util::reverse_board(const uint64_t board) {
    const uint64_t COL0 = (board & 63ULL);
    const uint64_t COL1 = (board & 16128ULL) >> 8ULL;
    const uint64_t COL2 = (board & 4128768ULL) >> 16ULL;
    const uint64_t COL3 = (board & 1056964608ULL) >> 24ULL;
    const uint64_t COL4 = (board & 270582939648ULL) >> 32ULL;
    const uint64_t COL5 = (board & 69269232549888ULL) >> 40ULL;
    const uint64_t COL6 = (board & 17732923532771328ULL) >> 48ULL;

    return COL6 | (COL5 << 8ULL) | (COL4 << 16ULL) | (COL3 << 24ULL)
           | (COL2 << 32ULL) | (COL1 << 40ULL) | (COL0 << 48ULL);
}


#endif //CONNECT4_UTIL_H
