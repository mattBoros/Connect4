
#ifndef CONNECT4_STATE_H
#define CONNECT4_STATE_H


#include "Piece.h"
#include "Time.h"

static const uint64_t MASKS[6][7] = {
        {1ULL,  256ULL,  65536ULL,   16777216ULL,  4294967296ULL,   1099511627776ULL,  281474976710656ULL}, // row 0
        {2ULL,  512ULL,  131072ULL,  33554432ULL,  8589934592ULL,   2199023255552ULL,  562949953421312ULL}, // row 1
        {4ULL,  1024ULL, 262144ULL,  67108864ULL,  17179869184ULL,  4398046511104ULL,  1125899906842624ULL}, // row 2
        {8ULL,  2048ULL, 524288ULL,  134217728ULL, 34359738368ULL,  8796093022208ULL,  2251799813685248ULL}, // row 3
        {16ULL, 4096ULL, 1048576ULL, 268435456ULL, 68719476736ULL,  17592186044416ULL, 4503599627370496ULL}, // row 4
        {32ULL, 8192ULL, 2097152ULL, 536870912ULL, 137438953472ULL, 35184372088832ULL, 9007199254740992ULL}}; // row 5


static inline uint64_t reverse_board(const uint64_t board) {
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

class State {
public:
    const uint64_t blackPieces;
    const uint64_t whitePieces;
    const uint32_t hash_index;
    const uint32_t reversed_hash_index;

    const uint8_t col0_height;
    const uint8_t col1_height;
    const uint8_t col2_height;
    const uint8_t col3_height;
    const uint8_t col4_height;
    const uint8_t col5_height;
    const uint8_t col6_height;

//    const uint64_t ORd_board;

//    const uint8_t numSpotsFilled;

    inline State(
            const uint64_t blackPieces,
            const uint64_t whitePieces,
            const uint32_t hash_index,
            const uint32_t reversed_hash_index,
            const uint8_t col0_height,
            const uint8_t col1_height,
            const uint8_t col2_height,
            const uint8_t col3_height,
            const uint8_t col4_height,
            const uint8_t col5_height,
            const uint8_t col6_height
    ) :
            blackPieces(blackPieces),
            whitePieces(whitePieces),
            hash_index(hash_index),
            reversed_hash_index(reversed_hash_index),
            col0_height(col0_height),
            col1_height(col1_height),
            col2_height(col2_height),
            col3_height(col3_height),
            col4_height(col4_height),
            col5_height(col5_height),
            col6_height(col6_height) {
    }

    inline State reverse() const {
        return State(
                reverse_board(blackPieces),
                reverse_board(whitePieces),
                reversed_hash_index,
                hash_index,
                col6_height,
                col5_height,
                col4_height,
                col3_height,
                col2_height,
                col1_height,
                col0_height
        );
    }

    inline void print() const {
        for (int row = 5; row >= 0; --row) {
            for (int col = 0; col < 7; ++col) {
                const uint64_t mask = MASKS[row][col];
                if (blackPieces & mask) {
                    cout << "B";
                } else if (whitePieces & mask) {
                    cout << "W";
                } else {
                    cout << "_";
                }
            }
            cout << endl;
        }
        cout << endl;
    }
};

namespace Helpers2 {
    static State getStartingBoard() {
        return State(
                0ULL,
                0ULL,
                33554432 - 1,
                33554432 - 1,
                0, 0, 0, 0, 0, 0, 0
//,0ULL
//                ,0
        );
    }
}

#endif //CONNECT4_STATE_H
