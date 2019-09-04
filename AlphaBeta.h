#include <iostream>

using namespace std;

#include <chrono>
#include <sys/time.h>
#include <climits>
#include <unordered_map>

using namespace std::chrono;

#ifndef CONNECT4_ALPHABETA_H
#define CONNECT4_ALPHABETA_H

#include "State.h"
#include "Piece.h"
#include "Util.h"
#include "StoredSearch.h"
#include "StoredResult.h"
#include "TTable.h"
#include "Cache.h"

static const bool USE_KILLER_MOVE = false;
static const bool COUNT_STATS = true;

//static const int8_t NEG_INFINITY = -127;
static const int8_t NEG_INFINITY = -1;
//static const int8_t POS_INFINITY = 127;
static const int8_t POS_INFINITY = 1;

static const uint64_t FULL_BOARD = 17802464409370431ULL;

static const uint8_t INDEX_LOOKUP_TABLE_ROW_COL[8][8] = {
        {0,  1,  2,  3,  4,  5,  6,  7},
        {8,  9,  10, 11, 12, 13, 14, 15},
        {16, 17, 18, 19, 20, 21, 22, 23},
        {24, 25, 26, 27, 28, 29, 30, 31},
        {32, 33, 34, 35, 36, 37, 38, 39},
        {40, 41, 42, 43, 44, 45, 46, 47},
        {48, 49, 50, 51, 52, 53, 54, 55},
        {56, 57, 58, 59, 60, 61, 62, 63},
};


static const uint8_t INDEX_LOOKUP_TABLE_COL_ROW[8][8] = {
        {0, 8,  16, 24, 32, 40, 48, 56},
        {1, 9,  17, 25, 33, 41, 49, 57},
        {2, 10, 18, 26, 34, 42, 50, 58},
        {3, 11, 19, 27, 35, 43, 51, 59},
        {4, 12, 20, 28, 36, 44, 52, 60},
        {5, 13, 21, 29, 37, 45, 53, 61},
        {6, 14, 22, 30, 38, 46, 54, 62},
        {7, 15, 23, 31, 39, 47, 55, 63},
};

static const uint8_t KILLER_MOVE_ORDERS[7][7] = {
        {0, 3, 2, 4, 1, 5, 6}, // km = 0
        {1, 3, 2, 4, 5, 0, 6}, // km = 1
        {2, 3, 4, 1, 5, 0, 6}, // km = 2
        {3, 2, 4, 1, 5, 0, 6}, // km = 3
        {4, 3, 2, 1, 5, 0, 6}, // km = 4
        {5, 3, 2, 4, 1, 0, 6}, // km = 5
        {6, 3, 2, 4, 1, 5, 0}  // km = 6
};

static const uint8_t DEFAULT_MOVE_ORDER[7] = {3, 2, 4, 1, 5, 0, 6};
//static const uint8_t MOVE_ORDER[7] = {0, 1, 2, 3, 4, 5, 6};

template<const uint8_t maxDepth, const bool side>
class AlphaBeta {
public:

    mutable uint8_t killer_moves[42] = {0};
    mutable TTable ttable;
    mutable Cache cache;

    inline AlphaBeta();

    inline const bool hasvalue(const uint64_t x, const uint64_t n) const
    __attribute__ ((hot, const, no_stack_limit));
 
    inline bool scoreSlow(const uint64_t pieces) const;

    template<const bool current_color>
    inline int8_t MTDF(const State node) const;

    template<const bool current_color, const uint8_t depth>
    int8_t const inline pvs_init(const State node,
                                 int8_t alpha, const int8_t beta,
                                 uint8_t *&move_to_make) const;

    template<const bool current_color, const uint8_t depth>
    int8_t const inline pvs(const State node,
                            const int8_t alpha, const int8_t beta) const
    __attribute__ ((hot, no_stack_limit));


    template<const bool current_color, const uint8_t depth>
    int8_t const inline pvs_no_ttable(const State node,
                            const int8_t alpha, const int8_t beta) const
    __attribute__ ((hot, no_stack_limit));

    template<const bool current_color>
    inline State applyAction(const uint64_t child_board, const State node, const uint8_t row, const uint8_t col) const
    __attribute__ ((const));


    template<const bool current_color, const uint8_t depth>
    inline int8_t pvs_no_ttable_no_cache(const State node,
                                         int8_t alpha, const int8_t beta) const
    __attribute__ ((hot, no_stack_limit));

    inline uint8_t getColHeight(const uint64_t ORd_board, const uint8_t col) const;

    inline uint8_t getColHeight(const State state, const uint8_t col) const __attribute__ ((const));

    inline const bool score(const uint64_t pieces, const uint8_t row, const uint8_t col) const
    __attribute__ ((hot, const));

};

template<const uint8_t maxDepth, const bool side>
inline AlphaBeta<maxDepth, side>::AlphaBeta() {
    for (uint8_t i = 0; i < 42; ++i) {
        killer_moves[i] = 3;
    }
//        ttable.reserve(TTABLE_BUCKET_SIZE);
//        ttable.max_load_factor(0.5);

//        for (int col = 0; col < 8; ++col) {
//            cout << "{";
//            for (int row = 0; row < 8; ++row) {
//                cout << (int) INDEX_LOOKUP_TABLE_ROW_COL[row][col] << ", ";
//            }
//            cout << "}," << endl;
//        }
}

template<const uint8_t maxDepth, const bool side>
inline const bool AlphaBeta<maxDepth, side>::hasvalue(const uint64_t x, const uint64_t n) const {
    return (x | ~n) == UINT64_MAX;
//        return (x & n) == n;
//        return !((x & n) - n);
}

template<const uint8_t maxDepth, const bool side>
inline bool AlphaBeta<maxDepth, side>::scoreSlow(const uint64_t pieces) const {
    for (int i = 0; i < 7; ++i) {
        for (int j = 0; j < 6; ++j) {
            if (score(pieces, j, i)) {
                return true;
            }
        }
    }
    return false;
}

template<const uint8_t maxDepth, const bool side>
template<const bool current_color>
int8_t AlphaBeta<maxDepth, side>::MTDF(const State node) const {
    int8_t g = 0;
    int8_t upperBound = POS_INFINITY;
    int8_t lowerBound = NEG_INFINITY;
    uint8_t *move_to_make = nullptr;
    while (lowerBound < upperBound) {
        int8_t beta = Util::max(g, lowerBound + 1);
        cout << "Lower bound " << (int) lowerBound << endl;
        cout << "Upper bound " << (int) upperBound << endl;
        cout << "beta " << (int) beta << endl;
        g = pvs_init<current_color, 0>(node, beta - 1, beta, move_to_make);
        if (g < beta) {
            upperBound = g;
        } else {
            lowerBound = g;
        }
    }
    cout << "score : " << (int) g << endl;
    return *move_to_make;
}


template<const uint8_t maxDepth, const bool side>
template<const bool current_color, const uint8_t depth>
int8_t const inline AlphaBeta<maxDepth, side>::pvs_init(const State node,
                                                        int8_t alpha, const int8_t beta,
                                                        uint8_t *&move_to_make) const {
    constexpr uint8_t next_depth = depth + 1;
    const uint64_t my_pieces = current_color ? node.blackPieces : node.whitePieces;
//        const uint64_t ORd_board = node.blackPieces | node.whitePieces;
    for (uint8_t i = 0; i < 7; ++i) {
        uint8_t col = DEFAULT_MOVE_ORDER[i];
        const uint8_t row = getColHeight(node, col);
        if (row != 6) {
            const uint64_t child_board = my_pieces | Util::MASKS[row][col];
            const State child = applyAction<current_color>(child_board, node, row, col);
            int8_t score;
            if (i == 0) {
                score = -pvs<!current_color, next_depth>(child, -beta, -alpha);
            } else {
                score = -pvs<!current_color, next_depth>(child, -alpha - 1, -alpha);
                if (alpha < score && score < beta) {
                    score = -pvs<!current_color, next_depth>(child, -beta, -score);
                }
            }
            if (score > alpha) {
                move_to_make = new uint8_t(col);
                alpha = score;
            }
            if (alpha >= beta) {
                return alpha;
            }
        }
    }
    return alpha;
}

template<const uint8_t maxDepth, const bool side>
template<const bool current_color, const uint8_t depth>
int8_t const inline AlphaBeta<maxDepth, side>::pvs(const State node,
                                                   const int8_t alpha, const int8_t beta) const {
    if (depth <= 31) {
        const StoredSearch key(
                node,
                alpha, beta
        );
        const uint32_t key_index = node.hash_index % TTABLE_BUCKET_SIZE;
        const StoredResult result = ttable.find(key_index, key);
        if (result.is_null()) { // doesnt exist in ttable
            if (COUNT_STATS) TIME::ttable_misses++;
            const int8_t v = pvs_no_ttable<current_color, depth>(node, alpha, beta);
            ttable.put(key_index, key, StoredResult(v));
            return v;
        } else {
            if (COUNT_STATS) TIME::ttable_hits++;
            return result.value;
        }
    } else {
        return pvs_no_ttable<current_color, depth>(node, alpha, beta);
    }
}

template<const uint8_t maxDepth, const bool side>
template<const bool current_color, const uint8_t depth>
int8_t const inline AlphaBeta<maxDepth, side>::pvs_no_ttable(const State node,
                                                             const int8_t alpha, const int8_t beta) const {

    // maxdepth = 35
    // depth <= 10 : 81.65
    // depth <= 12 : 72
    // depth <= 13 : 63.29
    // depth <= 14 : 62.257
    // depth <= 15 : 70.366

    if (depth <= 14) {
        if (__builtin_expect(cache.size() >= CACHE_CAPACITY, 0)) {
            cout << "cache cleared" << endl;
            cout << "old cache size : " << cache.size() << endl;
            cache.clear();
            cout << "new cache size : " << cache.size() << endl << endl;
        }

        const StoredSearch key(
                node,
                alpha, beta
        );
        const uint32_t key_index = node.hash_index % CACHE_BUCKET_SIZE;
        const StoredResult result = cache.find(key_index, key);
        if (result.is_null()) { // doesnt exist in cache
            if (COUNT_STATS) TIME::cache_misses++;
            const int8_t v = pvs_no_ttable_no_cache<current_color, depth>(node, alpha, beta);
            cache.put(key_index, key, StoredResult(v));
            return v;
        } else {
            if (COUNT_STATS) TIME::cache_hits++;
            return result.value;
        }
    } else {
        return pvs_no_ttable_no_cache<current_color, depth>(node, alpha, beta);
    }
}

template<const uint8_t maxDepth, const bool side>
template<const bool current_color, const uint8_t depth>
inline int8_t AlphaBeta<maxDepth, side>::pvs_no_ttable_no_cache(const State node,
                                                                int8_t alpha, const int8_t beta) const {
//        cout << (current_color ? "Black turn" : "White turn") << endl;
//        node.print();
    if (COUNT_STATS) TIME::positions_searched++;
    constexpr bool is_last_depth = depth == maxDepth;
    constexpr uint8_t next_depth = depth + 1;
    constexpr bool is_next_depth_last = next_depth == maxDepth;

    if (is_last_depth) {
        return 0;
    }

//        const uint64_t ORd_board = node.blackPieces | node.whitePieces;

//        const int8_t original_score = getWinner<!current_color>(node, prev_row, prev_col); // TODO: can remove this maybe
//        if (depth == maxDepth) {
//            return 0;
//        }

    // check if it can win in one turn
//        Util::print_ull(node.blackPieces);
//        Util::print_ull(node.whitePieces);
    uint8_t col_heights[7] = {6, 6, 6, 6, 6, 6, 6};
//        uint64_t board_with_pieces_placed[7] = {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX,
//                                                UINT64_MAX, UINT64_MAX, UINT64_MAX};
    const uint64_t my_pieces = current_color ? node.blackPieces : node.whitePieces;
    for (uint8_t col = 0; col < 7; ++col) {
        const uint8_t row = getColHeight(node, col);
        if (row != 6) {
//            if (__builtin_expect(row != 6, 1)) {
            if (COUNT_STATS) TIME::num_below_max_height_ifs++;
            const uint64_t new_board = my_pieces | Util::MASKS[row][col];
            if (score(new_board, row, col)) {
                if (COUNT_STATS) TIME::num_short_circuit_wins++;
                return 1;
            }
            col_heights[col] = row;
//                board_with_pieces_placed[col] = new_board;
        } else {
            if (COUNT_STATS) TIME::num_max_height_ifs++;
        }
    }

    const uint64_t other_pieces = current_color ? node.whitePieces : node.blackPieces;
    bool has_forced_move = false;
    uint8_t forced_move_col = 0;
    for (uint8_t col = 0; col < 7; ++col) {
        const uint8_t row = col_heights[col];
        if (row != 6) {
//            if (__builtin_expect(row != 6, 1)) {
            if (COUNT_STATS) TIME::num_below_max_height_ifs++;
            const uint64_t opposing_board = other_pieces | Util::MASKS[row][col];
            if (score(opposing_board, row, col)) { // if there is a forced move
                if (has_forced_move) {
                    if (COUNT_STATS) TIME::num_fm_two++;
                    // if num_forced_moves > 0 already, then it is >= 1. Then since we're incrementing it
                    // it will be >= 2, so we lose.
                    return -1;
                }
                has_forced_move = true;
                forced_move_col = col;
            }
        } else {
            if (COUNT_STATS) TIME::num_max_height_ifs++;
        }
    }

    if (has_forced_move) {
        if (COUNT_STATS) TIME::num_fm_one++;
        const uint8_t col = forced_move_col;
        const uint8_t row = col_heights[col];
        const uint64_t my_new_board = my_pieces | Util::MASKS[row][col];
        const State child = applyAction<current_color>(my_new_board, node, row, col);
        return -pvs<!current_color, next_depth>(child, -beta, -alpha);
    } else {
        if (COUNT_STATS) TIME::num_fm_zero++;
    }

    const uint8_t killer_move = USE_KILLER_MOVE ? killer_moves[depth] : 0;
    for (uint8_t i = 0; i < 7; ++i) {
        uint8_t col;
        // depth = 25
        // depth < 0 -> 51.576
        // depth < 1 -> 51.679
        // depth < 2 -> 52.907
        // depth < 3 -> 58.06
        // depth < 4 -> 75.124

        if (USE_KILLER_MOVE && depth <= 3) {
            col = KILLER_MOVE_ORDERS[killer_move][i];
        } else {
            col = DEFAULT_MOVE_ORDER[i];
        }

        const uint8_t row = col_heights[col];
//            if (row != 6) {
        if (__builtin_expect(row != 6, 1)) {
            if (COUNT_STATS) TIME::num_below_max_height_ifs++;
            const uint64_t child_board = my_pieces | Util::MASKS[row][col];
//                const State child = applyAction<current_color>(node, col,
//                                                               row); // TODO: change so it only copies one uint64
            const State child = applyAction<current_color>(child_board, node, row, col);
            int8_t score;
            if (i == 0) {
                score = -pvs<!current_color, next_depth>(child, -beta, -alpha);
            } else {
                score = -pvs<!current_color, next_depth>(child, -alpha - 1, -alpha);
                if (alpha < score && score < beta) {
                    if (COUNT_STATS) TIME::num_wrong_searches++;
                    score = -pvs<!current_color, next_depth>(child, -beta, -score);
                }
            }
            alpha = Util::max(alpha, score);
            if (__builtin_expect(alpha >= beta, 0)) {
//                if (alpha >= beta) {
                if (COUNT_STATS) TIME::cutoffs++;
                if (USE_KILLER_MOVE) killer_moves[depth] = col;
                return alpha;
            } else {
                if (COUNT_STATS) TIME::non_cutoffs++;
            }
        } else {
            if (COUNT_STATS) TIME::num_max_height_ifs++;
        }
    }
    return alpha;
}


template<const uint8_t maxDepth, const bool side>
template<const bool current_color>
inline State AlphaBeta<maxDepth, side>::applyAction(const uint64_t child_board, const State node, const uint8_t row,
                                                    const uint8_t col) const {
    switch (col) {
        case 0: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height + 1,
                         node.col1_height,
                         node.col2_height,
                         node.col3_height,
                         node.col4_height,
                         node.col5_height,
                         node.col6_height
            );
        }
        case 1: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height + 1,
                         node.col2_height,
                         node.col3_height,
                         node.col4_height,
                         node.col5_height,
                         node.col6_height
            );
        }
        case 2: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height,
                         node.col2_height + 1,
                         node.col3_height,
                         node.col4_height,
                         node.col5_height,
                         node.col6_height
            );
        }
        case 3: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height,
                         node.col2_height,
                         node.col3_height + 1,
                         node.col4_height,
                         node.col5_height,
                         node.col6_height
            );
        }
        case 4: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height,
                         node.col2_height,
                         node.col3_height,
                         node.col4_height + 1,
                         node.col5_height,
                         node.col6_height
            );
        }
        case 5: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height,
                         node.col2_height,
                         node.col3_height,
                         node.col4_height,
                         node.col5_height + 1,
                         node.col6_height
            );
        }
        case 6: {
            return State(current_color ? child_board : node.blackPieces,
                         current_color ? node.whitePieces : child_board,
                         node.hash_index ^ HASH_POSITIONS[current_color][row][col],
                         node.col0_height,
                         node.col1_height,
                         node.col2_height,
                         node.col3_height,
                         node.col4_height,
                         node.col5_height,
                         node.col6_height + 1
            );
        }
    }
}

template<const uint8_t maxDepth, const bool side>
inline uint8_t AlphaBeta<maxDepth, side>::getColHeight(const uint64_t ORd_board, const uint8_t col) const {
    switch (col) {
        case 0: {
            const uint64_t COL = ORd_board & 63ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 1ULL:
                    return 1;
                case 3ULL:
                    return 2;
                case 7ULL:
                    return 3;
                case 15ULL:
                    return 4;
                case 31ULL:
                    return 5;
                case 63ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 1: {
            const uint64_t COL = ORd_board & 16128ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 256ULL:
                    return 1;
                case 768ULL:
                    return 2;
                case 1792ULL:
                    return 3;
                case 3840ULL:
                    return 4;
                case 7936ULL:
                    return 5;
                case 16128ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 2: {
            const uint64_t COL = ORd_board & 4128768ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 65536ULL:
                    return 1;
                case 196608ULL:
                    return 2;
                case 458752ULL:
                    return 3;
                case 983040ULL:
                    return 4;
                case 2031616ULL:
                    return 5;
                case 4128768ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 3: {
            const uint64_t COL = ORd_board & 1056964608ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 16777216ULL:
                    return 1;
                case 50331648ULL:
                    return 2;
                case 117440512ULL:
                    return 3;
                case 251658240ULL:
                    return 4;
                case 520093696ULL:
                    return 5;
                case 1056964608ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 4: {
            const uint64_t COL = ORd_board & 270582939648ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 4294967296ULL:
                    return 1;
                case 12884901888ULL:
                    return 2;
                case 30064771072ULL:
                    return 3;
                case 64424509440ULL:
                    return 4;
                case 133143986176ULL:
                    return 5;
                case 270582939648ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 5: {
            const uint64_t COL = ORd_board & 69269232549888ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 1099511627776ULL:
                    return 1;
                case 3298534883328ULL:
                    return 2;
                case 7696581394432ULL:
                    return 3;
                case 16492674416640ULL:
                    return 4;
                case 34084860461056ULL:
                    return 5;
                case 69269232549888ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 6: {
            const uint64_t COL = ORd_board & 17732923532771328ULL;
            switch (COL) {
                case 0:
                    return 0;
                case 281474976710656ULL:
                    return 1;
                case 844424930131968ULL:
                    return 2;
                case 1970324836974592ULL:
                    return 3;
                case 4222124650659840ULL:
                    return 4;
                case 8725724278030336ULL:
                    return 5;
                case 17732923532771328ULL:
                    return 6;
                default: {
                    cout << "ERROR" << endl;
                    exit(0);
                }
            }
        }
        case 7:
            return 6;
        case UINT8_MAX:
            return 6;
        default: {
            cout << "error 13213213" << endl;
            exit(0);
        }
    }
}

template<const uint8_t maxDepth, const bool side>
inline uint8_t AlphaBeta<maxDepth, side>::getColHeight(const State state, const uint8_t col) const {
    // XXXXXX
    // XXXXXX
    // XXXXXX
    // XXXXXX
    // XXXXXX
    // XXXXXX
    // XXXXXX
    switch (col) {
        case 0: {
            return state.col0_height;
        }
        case 1: {
            return state.col1_height;
        }
        case 2: {
            return state.col2_height;
        }
        case 3: {
            return state.col3_height;
        }
        case 4: {
            return state.col4_height;
        }
        case 5: {
            return state.col5_height;
        }
        case 6: {
            return state.col6_height;
        }
    }
}

static const bool BRANCHLESS_SCORE = true;

template<const uint8_t maxDepth, const bool side>
inline const bool AlphaBeta<maxDepth, side>::score(const uint64_t pieces, const uint8_t row, const uint8_t col) const {
    if (BRANCHLESS_SCORE) {
        const uint8_t index = INDEX_LOOKUP_TABLE_ROW_COL[row][col];
        const bool index_24_comp = index <= 24;
//        return (
//                (index_24_comp &&
//                 (
//                         (index <= 11 && (
//                                 (
//                                         (index == 0 && (
//                                                 hasvalue(pieces, 15ULL) | hasvalue(pieces, 16843009ULL) |
//                                                 hasvalue(pieces, 134480385ULL))) |
//                                         (index == 1 && (
//                                                 hasvalue(pieces, 3840ULL) | hasvalue(pieces, 16843009ULL) |
//                                                 hasvalue(pieces, 4311810304ULL) |
//                                                 hasvalue(pieces, 34426978560ULL))) |
//                                         (index == 2 && (
//                                                 hasvalue(pieces, 983040ULL) | hasvalue(pieces, 16843009ULL) |
//                                                 hasvalue(pieces, 4311810304ULL) |
//                                                 hasvalue(pieces, 1103823437824ULL) |
//                                                 hasvalue(pieces, 8813306511360ULL))) |
//                                         (index == 3 && (
//                                                 hasvalue(pieces, 251658240ULL) | hasvalue(pieces, 16843009ULL) |
//                                                 hasvalue(pieces, 4311810304ULL) |
//                                                 hasvalue(pieces, 1103823437824ULL) |
//                                                 hasvalue(pieces, 282578800082944ULL) |
//                                                 hasvalue(pieces, 2256206466908160ULL) |
//                                                 hasvalue(pieces, 16909320ULL))) |
//                                         (index == 4 && (
//                                                 hasvalue(pieces, 64424509440ULL) | hasvalue(pieces, 4311810304ULL) |
//                                                 hasvalue(pieces, 1103823437824ULL) |
//                                                 hasvalue(pieces, 282578800082944ULL) |
//                                                 hasvalue(pieces, 4328785920ULL))) |
//                                         (index == 5 && (
//                                                 hasvalue(pieces, 16492674416640ULL) |
//                                                 hasvalue(pieces, 1103823437824ULL) |
//                                                 hasvalue(pieces, 282578800082944ULL) |
//                                                 hasvalue(pieces, 1108169195520ULL))) |
//                                         (index == 6 && (
//                                                 hasvalue(pieces, 4222124650659840ULL) |
//                                                 hasvalue(pieces, 282578800082944ULL) |
//                                                 hasvalue(pieces, 283691314053120ULL))) |
//                                         (index == 8 && (
//                                                 hasvalue(pieces, 15ULL) | hasvalue(pieces, 30ULL) |
//                                                 hasvalue(pieces, 33686018ULL) |
//                                                 hasvalue(pieces, 268960770ULL))) |
//                                         (index == 9 && (
//                                                 hasvalue(pieces, 3840ULL) | hasvalue(pieces, 7680ULL) |
//                                                 hasvalue(pieces, 33686018ULL) |
//                                                 hasvalue(pieces, 8623620608ULL) | hasvalue(pieces, 134480385ULL) |
//                                                 hasvalue(pieces, 68853957120ULL))) |
//                                         (index == 10 && (
//                                                 hasvalue(pieces, 983040ULL) | hasvalue(pieces, 1966080ULL) |
//                                                 hasvalue(pieces, 33686018ULL) |
//                                                 hasvalue(pieces, 8623620608ULL) |
//                                                 hasvalue(pieces, 2207646875648ULL) |
//                                                 hasvalue(pieces, 34426978560ULL) |
//                                                 hasvalue(pieces, 17626613022720ULL) |
//                                                 hasvalue(pieces, 16909320ULL))) |
//                                         (index == 11 && (
//                                                 hasvalue(pieces, 251658240ULL) | hasvalue(pieces, 503316480ULL) |
//                                                 hasvalue(pieces, 33686018ULL) |
//                                                 hasvalue(pieces, 8623620608ULL) |
//                                                 hasvalue(pieces, 2207646875648ULL) |
//                                                 hasvalue(pieces, 565157600165888ULL) |
//                                                 hasvalue(pieces, 8813306511360ULL) |
//                                                 hasvalue(pieces, 4512412933816320ULL) |
//                                                 hasvalue(pieces, 4328785920ULL) |
//                                                 hasvalue(pieces, 33818640ULL)))
//                                 )
//                         ))
//                         ||
//                         (index >= 12 && (
//                                 (index == 12 && (
//                                         hasvalue(pieces, 64424509440ULL) | hasvalue(pieces, 128849018880ULL) |
//                                         hasvalue(pieces, 8623620608ULL) | hasvalue(pieces, 2207646875648ULL) |
//                                         hasvalue(pieces, 565157600165888ULL) |
//                                         hasvalue(pieces, 2256206466908160ULL) |
//                                         hasvalue(pieces, 1108169195520ULL) | hasvalue(pieces, 8657571840ULL))) |
//                                 (index == 13 && (
//                                         hasvalue(pieces, 16492674416640ULL) | hasvalue(pieces, 32985348833280ULL) |
//                                         hasvalue(pieces, 2207646875648ULL) | hasvalue(pieces, 565157600165888ULL) |
//                                         hasvalue(pieces, 283691314053120ULL) | hasvalue(pieces, 2216338391040ULL))) |
//                                 (index == 14 && (
//                                         hasvalue(pieces, 4222124650659840ULL) |
//                                         hasvalue(pieces, 8444249301319680ULL) |
//                                         hasvalue(pieces, 565157600165888ULL) |
//                                         hasvalue(pieces, 567382628106240ULL))) |
//                                 (index == 16 && (
//                                         hasvalue(pieces, 15ULL) | hasvalue(pieces, 30ULL) |
//                                         hasvalue(pieces, 60ULL) |
//                                         hasvalue(pieces, 67372036ULL) | hasvalue(pieces, 537921540ULL))) |
//                                 (index == 17 && (
//                                         hasvalue(pieces, 3840ULL) | hasvalue(pieces, 7680ULL) |
//                                         hasvalue(pieces, 15360ULL) |
//                                         hasvalue(pieces, 67372036ULL) | hasvalue(pieces, 17247241216ULL) |
//                                         hasvalue(pieces, 268960770ULL) | hasvalue(pieces, 137707914240ULL) |
//                                         hasvalue(pieces, 16909320ULL))) |
//                                 (index == 18 && (
//                                         hasvalue(pieces, 983040ULL) | hasvalue(pieces, 1966080ULL) |
//                                         hasvalue(pieces, 3932160ULL) |
//                                         hasvalue(pieces, 67372036ULL) | hasvalue(pieces, 17247241216ULL) |
//                                         hasvalue(pieces, 4415293751296ULL) | hasvalue(pieces, 134480385ULL) |
//                                         hasvalue(pieces, 68853957120ULL) | hasvalue(pieces, 35253226045440ULL) |
//                                         hasvalue(pieces, 4328785920ULL) | hasvalue(pieces, 33818640ULL))) |
//                                 (index == 19 && (
//                                         hasvalue(pieces, 251658240ULL) | hasvalue(pieces, 503316480ULL) |
//                                         hasvalue(pieces, 1006632960ULL) | hasvalue(pieces, 67372036ULL) |
//                                         hasvalue(pieces, 17247241216ULL) | hasvalue(pieces, 4415293751296ULL) |
//                                         hasvalue(pieces, 1130315200331776ULL) | hasvalue(pieces, 34426978560ULL) |
//                                         hasvalue(pieces, 17626613022720ULL) | hasvalue(pieces, 9024825867632640ULL) |
//                                         hasvalue(pieces, 1108169195520ULL) | hasvalue(pieces, 8657571840ULL) |
//                                         hasvalue(pieces, 67637280ULL))) |
//                                 (index == 20 && (
//                                         hasvalue(pieces, 64424509440ULL) | hasvalue(pieces, 128849018880ULL) |
//                                         hasvalue(pieces, 257698037760ULL) | hasvalue(pieces, 17247241216ULL) |
//                                         hasvalue(pieces, 4415293751296ULL) | hasvalue(pieces, 1130315200331776ULL) |
//                                         hasvalue(pieces, 8813306511360ULL) | hasvalue(pieces, 4512412933816320ULL) |
//                                         hasvalue(pieces, 283691314053120ULL) | hasvalue(pieces, 2216338391040ULL) |
//                                         hasvalue(pieces, 17315143680ULL))) |
//                                 (index == 21 && (
//                                         hasvalue(pieces, 16492674416640ULL) | hasvalue(pieces, 32985348833280ULL) |
//                                         hasvalue(pieces, 65970697666560ULL) | hasvalue(pieces, 4415293751296ULL) |
//                                         hasvalue(pieces, 1130315200331776ULL) |
//                                         hasvalue(pieces, 2256206466908160ULL) |
//                                         hasvalue(pieces, 567382628106240ULL) | hasvalue(pieces, 4432676782080ULL))) |
//                                 (index == 22 && (
//                                         hasvalue(pieces, 4222124650659840ULL) |
//                                         hasvalue(pieces, 8444249301319680ULL) |
//                                         hasvalue(pieces, 16888498602639360ULL) |
//                                         hasvalue(pieces, 1130315200331776ULL) |
//                                         hasvalue(pieces, 1134765256212480ULL))) |
//                                 (index == 24 && (
//                                         hasvalue(pieces, 15ULL) | hasvalue(pieces, 30ULL) |
//                                         hasvalue(pieces, 60ULL) |
//                                         hasvalue(pieces, 134744072ULL) | hasvalue(pieces, 16909320ULL))))
//                         ))
//
//                )
//                ||
//                (!index_24_comp &&
//                 (
//                         (index <= 33 &&
//                          (
//                                  (index == 25 && (
//                                          hasvalue(pieces, 3840ULL) | hasvalue(pieces, 7680ULL) |
//                                          hasvalue(pieces, 15360ULL) |
//                                          hasvalue(pieces, 134744072ULL) | hasvalue(pieces, 34494482432ULL) |
//                                          hasvalue(pieces, 537921540ULL) | hasvalue(pieces, 4328785920ULL) |
//                                          hasvalue(pieces, 33818640ULL))) |
//                                  (index == 26 && (
//                                          hasvalue(pieces, 983040ULL) | hasvalue(pieces, 1966080ULL) |
//                                          hasvalue(pieces, 3932160ULL) |
//                                          hasvalue(pieces, 134744072ULL) | hasvalue(pieces, 34494482432ULL) |
//                                          hasvalue(pieces, 8830587502592ULL) | hasvalue(pieces, 268960770ULL) |
//                                          hasvalue(pieces, 137707914240ULL) | hasvalue(pieces, 1108169195520ULL) |
//                                          hasvalue(pieces, 8657571840ULL) | hasvalue(pieces, 67637280ULL))) |
//                                  (index == 27 && (
//                                          hasvalue(pieces, 251658240ULL) | hasvalue(pieces, 503316480ULL) |
//                                          hasvalue(pieces, 1006632960ULL) | hasvalue(pieces, 134744072ULL) |
//                                          hasvalue(pieces, 34494482432ULL) | hasvalue(pieces, 8830587502592ULL) |
//                                          hasvalue(pieces, 2260630400663552ULL) | hasvalue(pieces, 134480385ULL) |
//                                          hasvalue(pieces, 68853957120ULL) | hasvalue(pieces, 35253226045440ULL) |
//                                          hasvalue(pieces, 283691314053120ULL) | hasvalue(pieces, 2216338391040ULL) |
//                                          hasvalue(pieces, 17315143680ULL))) |
//                                  (index == 28 && (
//                                          hasvalue(pieces, 64424509440ULL) | hasvalue(pieces, 128849018880ULL) |
//                                          hasvalue(pieces, 257698037760ULL) | hasvalue(pieces, 34494482432ULL) |
//                                          hasvalue(pieces, 8830587502592ULL) | hasvalue(pieces, 2260630400663552ULL) |
//                                          hasvalue(pieces, 34426978560ULL) | hasvalue(pieces, 17626613022720ULL) |
//                                          hasvalue(pieces, 9024825867632640ULL) |
//                                          hasvalue(pieces, 567382628106240ULL) |
//                                          hasvalue(pieces, 4432676782080ULL))) |
//                                  (index == 29 && (
//                                          hasvalue(pieces, 16492674416640ULL) | hasvalue(pieces, 32985348833280ULL) |
//                                          hasvalue(pieces, 65970697666560ULL) | hasvalue(pieces, 8830587502592ULL) |
//                                          hasvalue(pieces, 2260630400663552ULL) | hasvalue(pieces, 8813306511360ULL) |
//                                          hasvalue(pieces, 4512412933816320ULL) |
//                                          hasvalue(pieces, 1134765256212480ULL))) |
//                                  (index == 30 && (
//                                          hasvalue(pieces, 4222124650659840ULL) |
//                                          hasvalue(pieces, 8444249301319680ULL) |
//                                          hasvalue(pieces, 16888498602639360ULL) |
//                                          hasvalue(pieces, 2260630400663552ULL) |
//                                          hasvalue(pieces, 2256206466908160ULL))) |
//                                  (index == 32 && (
//                                          hasvalue(pieces, 30ULL) | hasvalue(pieces, 60ULL) |
//                                          hasvalue(pieces, 269488144ULL) |
//                                          hasvalue(pieces, 33818640ULL))) |
//                                  (index == 33 && (
//                                          hasvalue(pieces, 7680ULL) | hasvalue(pieces, 15360ULL) |
//                                          hasvalue(pieces, 269488144ULL) |
//                                          hasvalue(pieces, 68988964864ULL) | hasvalue(pieces, 8657571840ULL) |
//                                          hasvalue(pieces, 67637280ULL))))
//                         )
//                         ||
//                         (index >= 34 && (
//                                 (index == 34 && (
//                                         hasvalue(pieces, 1966080ULL) | hasvalue(pieces, 3932160ULL) |
//                                         hasvalue(pieces, 269488144ULL) |
//                                         hasvalue(pieces, 68988964864ULL) | hasvalue(pieces, 17661175005184ULL) |
//                                         hasvalue(pieces, 537921540ULL) | hasvalue(pieces, 2216338391040ULL) |
//                                         hasvalue(pieces, 17315143680ULL))) |
//                                 (index == 35 && (
//                                         hasvalue(pieces, 503316480ULL) | hasvalue(pieces, 1006632960ULL) |
//                                         hasvalue(pieces, 269488144ULL) | hasvalue(pieces, 68988964864ULL) |
//                                         hasvalue(pieces, 17661175005184ULL) | hasvalue(pieces, 4521260801327104ULL) |
//                                         hasvalue(pieces, 268960770ULL) | hasvalue(pieces, 137707914240ULL) |
//                                         hasvalue(pieces, 567382628106240ULL) | hasvalue(pieces, 4432676782080ULL))) |
//                                 (index == 36 && (
//                                         hasvalue(pieces, 128849018880ULL) | hasvalue(pieces, 257698037760ULL) |
//                                         hasvalue(pieces, 68988964864ULL) | hasvalue(pieces, 17661175005184ULL) |
//                                         hasvalue(pieces, 4521260801327104ULL) | hasvalue(pieces, 68853957120ULL) |
//                                         hasvalue(pieces, 35253226045440ULL) |
//                                         hasvalue(pieces, 1134765256212480ULL))) |
//                                 (index == 37 && (
//                                         hasvalue(pieces, 32985348833280ULL) | hasvalue(pieces, 65970697666560ULL) |
//                                         hasvalue(pieces, 17661175005184ULL) | hasvalue(pieces, 4521260801327104ULL) |
//                                         hasvalue(pieces, 17626613022720ULL) |
//                                         hasvalue(pieces, 9024825867632640ULL))) |
//                                 (index == 38 && (
//                                         hasvalue(pieces, 8444249301319680ULL) |
//                                         hasvalue(pieces, 16888498602639360ULL) |
//                                         hasvalue(pieces, 4521260801327104ULL) |
//                                         hasvalue(pieces, 4512412933816320ULL))) |
//                                 (index == 40 && (
//                                         hasvalue(pieces, 60ULL) | hasvalue(pieces, 538976288ULL) |
//                                         hasvalue(pieces, 67637280ULL))) |
//                                 (index == 41 && (
//                                         hasvalue(pieces, 15360ULL) | hasvalue(pieces, 538976288ULL) |
//                                         hasvalue(pieces, 137977929728ULL) |
//                                         hasvalue(pieces, 17315143680ULL))) |
//                                 (index == 42 && (
//                                         hasvalue(pieces, 3932160ULL) | hasvalue(pieces, 538976288ULL) |
//                                         hasvalue(pieces, 137977929728ULL) | hasvalue(pieces, 35322350010368ULL) |
//                                         hasvalue(pieces, 4432676782080ULL))) |
//                                 (index == 43 && (
//                                         hasvalue(pieces, 1006632960ULL) | hasvalue(pieces, 538976288ULL) |
//                                         hasvalue(pieces, 137977929728ULL) | hasvalue(pieces, 35322350010368ULL) |
//                                         hasvalue(pieces, 9042521602654208ULL) | hasvalue(pieces, 537921540ULL) |
//                                         hasvalue(pieces, 1134765256212480ULL))) |
//                                 (index == 44 && (
//                                         hasvalue(pieces, 257698037760ULL) | hasvalue(pieces, 137977929728ULL) |
//                                         hasvalue(pieces, 35322350010368ULL) | hasvalue(pieces, 9042521602654208ULL) |
//                                         hasvalue(pieces, 137707914240ULL))) |
//                                 (index == 45 && (
//                                         hasvalue(pieces, 65970697666560ULL) | hasvalue(pieces, 35322350010368ULL) |
//                                         hasvalue(pieces, 9042521602654208ULL) |
//                                         hasvalue(pieces, 35253226045440ULL))) |
//                                 (index == 46 && (
//                                         hasvalue(pieces, 16888498602639360ULL) |
//                                         hasvalue(pieces, 9042521602654208ULL) |
//                                         hasvalue(pieces, 9024825867632640ULL)))))
//                 )
//                )
//        );
        return (
                (index_24_comp &&
                 (
                         (index <= 11 && (
                                 (
                                         (index == 0 && (
                                                 hasvalue(pieces, 15ULL) || hasvalue(pieces, 16843009ULL) ||
                                                 hasvalue(pieces, 134480385ULL))) ||
                                         (index == 1 && (
                                                 hasvalue(pieces, 3840ULL) || hasvalue(pieces, 16843009ULL) ||
                                                 hasvalue(pieces, 4311810304ULL) ||
                                                 hasvalue(pieces, 34426978560ULL))) ||
                                         (index == 2 && (
                                                 hasvalue(pieces, 983040ULL) || hasvalue(pieces, 16843009ULL) ||
                                                 hasvalue(pieces, 4311810304ULL) ||
                                                 hasvalue(pieces, 1103823437824ULL) ||
                                                 hasvalue(pieces, 8813306511360ULL))) ||
                                         (index == 3 && (
                                                 hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 16843009ULL) ||
                                                 hasvalue(pieces, 4311810304ULL) ||
                                                 hasvalue(pieces, 1103823437824ULL) ||
                                                 hasvalue(pieces, 282578800082944ULL) ||
                                                 hasvalue(pieces, 2256206466908160ULL) ||
                                                 hasvalue(pieces, 16909320ULL))) ||
                                         (index == 4 && (
                                                 hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 4311810304ULL) ||
                                                 hasvalue(pieces, 1103823437824ULL) ||
                                                 hasvalue(pieces, 282578800082944ULL) ||
                                                 hasvalue(pieces, 4328785920ULL))) ||
                                         (index == 5 && (
                                                 hasvalue(pieces, 16492674416640ULL) ||
                                                 hasvalue(pieces, 1103823437824ULL) ||
                                                 hasvalue(pieces, 282578800082944ULL) ||
                                                 hasvalue(pieces, 1108169195520ULL))) ||
                                         (index == 6 && (
                                                 hasvalue(pieces, 4222124650659840ULL) ||
                                                 hasvalue(pieces, 282578800082944ULL) ||
                                                 hasvalue(pieces, 283691314053120ULL))) ||
                                         (index == 8 && (
                                                 hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) ||
                                                 hasvalue(pieces, 33686018ULL) ||
                                                 hasvalue(pieces, 268960770ULL))) ||
                                         (index == 9 && (
                                                 hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) ||
                                                 hasvalue(pieces, 33686018ULL) ||
                                                 hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 134480385ULL) ||
                                                 hasvalue(pieces, 68853957120ULL))) ||
                                         (index == 10 && (
                                                 hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) ||
                                                 hasvalue(pieces, 33686018ULL) ||
                                                 hasvalue(pieces, 8623620608ULL) ||
                                                 hasvalue(pieces, 2207646875648ULL) ||
                                                 hasvalue(pieces, 34426978560ULL) ||
                                                 hasvalue(pieces, 17626613022720ULL) ||
                                                 hasvalue(pieces, 16909320ULL))) ||
                                         (index == 11 && (
                                                 hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                                                 hasvalue(pieces, 33686018ULL) ||
                                                 hasvalue(pieces, 8623620608ULL) ||
                                                 hasvalue(pieces, 2207646875648ULL) ||
                                                 hasvalue(pieces, 565157600165888ULL) ||
                                                 hasvalue(pieces, 8813306511360ULL) ||
                                                 hasvalue(pieces, 4512412933816320ULL) ||
                                                 hasvalue(pieces, 4328785920ULL) ||
                                                 hasvalue(pieces, 33818640ULL)))
                                 )
                         ))
                         ||
                         (index >= 12 && (
                                 (index == 12 && (
                                         hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                                         hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
                                         hasvalue(pieces, 565157600165888ULL) ||
                                         hasvalue(pieces, 2256206466908160ULL) ||
                                         hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL))) ||
                                 (index == 13 && (
                                         hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                                         hasvalue(pieces, 2207646875648ULL) || hasvalue(pieces, 565157600165888ULL) ||
                                         hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL))) ||
                                 (index == 14 && (
                                         hasvalue(pieces, 4222124650659840ULL) ||
                                         hasvalue(pieces, 8444249301319680ULL) ||
                                         hasvalue(pieces, 565157600165888ULL) ||
                                         hasvalue(pieces, 567382628106240ULL))) ||
                                 (index == 16 && (
                                         hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) ||
                                         hasvalue(pieces, 60ULL) ||
                                         hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 537921540ULL))) ||
                                 (index == 17 && (
                                         hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) ||
                                         hasvalue(pieces, 15360ULL) ||
                                         hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
                                         hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
                                         hasvalue(pieces, 16909320ULL))) ||
                                 (index == 18 && (
                                         hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) ||
                                         hasvalue(pieces, 3932160ULL) ||
                                         hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
                                         hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 134480385ULL) ||
                                         hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
                                         hasvalue(pieces, 4328785920ULL) || hasvalue(pieces, 33818640ULL))) ||
                                 (index == 19 && (
                                         hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                                         hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 67372036ULL) ||
                                         hasvalue(pieces, 17247241216ULL) || hasvalue(pieces, 4415293751296ULL) ||
                                         hasvalue(pieces, 1130315200331776ULL) || hasvalue(pieces, 34426978560ULL) ||
                                         hasvalue(pieces, 17626613022720ULL) || hasvalue(pieces, 9024825867632640ULL) ||
                                         hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL) ||
                                         hasvalue(pieces, 67637280ULL))) ||
                                 (index == 20 && (
                                         hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                                         hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 17247241216ULL) ||
                                         hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 1130315200331776ULL) ||
                                         hasvalue(pieces, 8813306511360ULL) || hasvalue(pieces, 4512412933816320ULL) ||
                                         hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
                                         hasvalue(pieces, 17315143680ULL))) ||
                                 (index == 21 && (
                                         hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                                         hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 4415293751296ULL) ||
                                         hasvalue(pieces, 1130315200331776ULL) ||
                                         hasvalue(pieces, 2256206466908160ULL) ||
                                         hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL))) ||
                                 (index == 22 && (
                                         hasvalue(pieces, 4222124650659840ULL) ||
                                         hasvalue(pieces, 8444249301319680ULL) ||
                                         hasvalue(pieces, 16888498602639360ULL) ||
                                         hasvalue(pieces, 1130315200331776ULL) ||
                                         hasvalue(pieces, 1134765256212480ULL))) ||
                                 (index == 24 && (
                                         hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) ||
                                         hasvalue(pieces, 60ULL) ||
                                         hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 16909320ULL))))
                         ))

                )
                ||
                (!index_24_comp &&
                 (
                         (index <= 33 &&
                          (
                                  (index == 25 && (
                                          hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) ||
                                          hasvalue(pieces, 15360ULL) ||
                                          hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
                                          hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 4328785920ULL) ||
                                          hasvalue(pieces, 33818640ULL))) ||
                                  (index == 26 && (
                                          hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) ||
                                          hasvalue(pieces, 3932160ULL) ||
                                          hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
                                          hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 268960770ULL) ||
                                          hasvalue(pieces, 137707914240ULL) || hasvalue(pieces, 1108169195520ULL) ||
                                          hasvalue(pieces, 8657571840ULL) || hasvalue(pieces, 67637280ULL))) ||
                                  (index == 27 && (
                                          hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                                          hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 134744072ULL) ||
                                          hasvalue(pieces, 34494482432ULL) || hasvalue(pieces, 8830587502592ULL) ||
                                          hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 134480385ULL) ||
                                          hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
                                          hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
                                          hasvalue(pieces, 17315143680ULL))) ||
                                  (index == 28 && (
                                          hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                                          hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 34494482432ULL) ||
                                          hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 2260630400663552ULL) ||
                                          hasvalue(pieces, 34426978560ULL) || hasvalue(pieces, 17626613022720ULL) ||
                                          hasvalue(pieces, 9024825867632640ULL) ||
                                          hasvalue(pieces, 567382628106240ULL) ||
                                          hasvalue(pieces, 4432676782080ULL))) ||
                                  (index == 29 && (
                                          hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                                          hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 8830587502592ULL) ||
                                          hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 8813306511360ULL) ||
                                          hasvalue(pieces, 4512412933816320ULL) ||
                                          hasvalue(pieces, 1134765256212480ULL))) ||
                                  (index == 30 && (
                                          hasvalue(pieces, 4222124650659840ULL) ||
                                          hasvalue(pieces, 8444249301319680ULL) ||
                                          hasvalue(pieces, 16888498602639360ULL) ||
                                          hasvalue(pieces, 2260630400663552ULL) ||
                                          hasvalue(pieces, 2256206466908160ULL))) ||
                                  (index == 32 && (
                                          hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) ||
                                          hasvalue(pieces, 269488144ULL) ||
                                          hasvalue(pieces, 33818640ULL))) ||
                                  (index == 33 && (
                                          hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) ||
                                          hasvalue(pieces, 269488144ULL) ||
                                          hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 8657571840ULL) ||
                                          hasvalue(pieces, 67637280ULL))))
                         )
                         ||
                         (index >= 34 && (
                                 (index == 34 && (
                                         hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) ||
                                         hasvalue(pieces, 269488144ULL) ||
                                         hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
                                         hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 2216338391040ULL) ||
                                         hasvalue(pieces, 17315143680ULL))) ||
                                 (index == 35 && (
                                         hasvalue(pieces, 503316480ULL) || hasvalue(pieces, 1006632960ULL) ||
                                         hasvalue(pieces, 269488144ULL) || hasvalue(pieces, 68988964864ULL) ||
                                         hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
                                         hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
                                         hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL))) ||
                                 (index == 36 && (
                                         hasvalue(pieces, 128849018880ULL) || hasvalue(pieces, 257698037760ULL) ||
                                         hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
                                         hasvalue(pieces, 4521260801327104ULL) || hasvalue(pieces, 68853957120ULL) ||
                                         hasvalue(pieces, 35253226045440ULL) ||
                                         hasvalue(pieces, 1134765256212480ULL))) ||
                                 (index == 37 && (
                                         hasvalue(pieces, 32985348833280ULL) || hasvalue(pieces, 65970697666560ULL) ||
                                         hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
                                         hasvalue(pieces, 17626613022720ULL) ||
                                         hasvalue(pieces, 9024825867632640ULL))) ||
                                 (index == 38 && (
                                         hasvalue(pieces, 8444249301319680ULL) ||
                                         hasvalue(pieces, 16888498602639360ULL) ||
                                         hasvalue(pieces, 4521260801327104ULL) ||
                                         hasvalue(pieces, 4512412933816320ULL))) ||
                                 (index == 40 && (
                                         hasvalue(pieces, 60ULL) || hasvalue(pieces, 538976288ULL) ||
                                         hasvalue(pieces, 67637280ULL))) ||
                                 (index == 41 && (
                                         hasvalue(pieces, 15360ULL) || hasvalue(pieces, 538976288ULL) ||
                                         hasvalue(pieces, 137977929728ULL) ||
                                         hasvalue(pieces, 17315143680ULL))) ||
                                 (index == 42 && (
                                         hasvalue(pieces, 3932160ULL) || hasvalue(pieces, 538976288ULL) ||
                                         hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
                                         hasvalue(pieces, 4432676782080ULL))) ||
                                 (index == 43 && (
                                         hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 538976288ULL) ||
                                         hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
                                         hasvalue(pieces, 9042521602654208ULL) || hasvalue(pieces, 537921540ULL) ||
                                         hasvalue(pieces, 1134765256212480ULL))) ||
                                 (index == 44 && (
                                         hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 137977929728ULL) ||
                                         hasvalue(pieces, 35322350010368ULL) || hasvalue(pieces, 9042521602654208ULL) ||
                                         hasvalue(pieces, 137707914240ULL))) ||
                                 (index == 45 && (
                                         hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 35322350010368ULL) ||
                                         hasvalue(pieces, 9042521602654208ULL) ||
                                         hasvalue(pieces, 35253226045440ULL))) ||
                                 (index == 46 && (
                                         hasvalue(pieces, 16888498602639360ULL) ||
                                         hasvalue(pieces, 9042521602654208ULL) ||
                                         hasvalue(pieces, 9024825867632640ULL)))))
                 )
                )
        );


//        const uint8_t index = INDEX_LOOKUP_TABLE_ROW_COL[row][col];
//        return (index == 0 && (
//                hasvalue(pieces, 15ULL) || hasvalue(pieces, 16843009ULL) || hasvalue(pieces, 134480385ULL))) ||
//               (index == 1 && (
//                       hasvalue(pieces, 3840ULL) || hasvalue(pieces, 16843009ULL) || hasvalue(pieces, 4311810304ULL) ||
//                       hasvalue(pieces, 34426978560ULL))) ||
//               (index == 2 && (
//                       hasvalue(pieces, 983040ULL) || hasvalue(pieces, 16843009ULL) ||
//                       hasvalue(pieces, 4311810304ULL) ||
//                       hasvalue(pieces, 1103823437824ULL) || hasvalue(pieces, 8813306511360ULL))) ||
//               (index == 3 && (
//                       hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 16843009ULL) ||
//                       hasvalue(pieces, 4311810304ULL) ||
//                       hasvalue(pieces, 1103823437824ULL) || hasvalue(pieces, 282578800082944ULL) ||
//                       hasvalue(pieces, 2256206466908160ULL) || hasvalue(pieces, 16909320ULL))) ||
//               (index == 4 && (
//                       hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 4311810304ULL) ||
//                       hasvalue(pieces, 1103823437824ULL) || hasvalue(pieces, 282578800082944ULL) ||
//                       hasvalue(pieces, 4328785920ULL))) ||
//               (index == 5 && (
//                       hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 1103823437824ULL) ||
//                       hasvalue(pieces, 282578800082944ULL) || hasvalue(pieces, 1108169195520ULL))) ||
//               (index == 6 && (
//                       hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 282578800082944ULL) ||
//                       hasvalue(pieces, 283691314053120ULL))) ||
//               (index == 8 && (
//                       hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 33686018ULL) ||
//                       hasvalue(pieces, 268960770ULL))) ||
//               (index == 9 && (
//                       hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 33686018ULL) ||
//                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 134480385ULL) ||
//                       hasvalue(pieces, 68853957120ULL))) ||
//               (index == 10 && (
//                       hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 33686018ULL) ||
//                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
//                       hasvalue(pieces, 34426978560ULL) || hasvalue(pieces, 17626613022720ULL) ||
//                       hasvalue(pieces, 16909320ULL))) ||
//               (index == 11 && (
//                       hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
//                       hasvalue(pieces, 33686018ULL) ||
//                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
//                       hasvalue(pieces, 565157600165888ULL) || hasvalue(pieces, 8813306511360ULL) ||
//                       hasvalue(pieces, 4512412933816320ULL) || hasvalue(pieces, 4328785920ULL) ||
//                       hasvalue(pieces, 33818640ULL))) ||
//               (index == 12 && (
//                       hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
//                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
//                       hasvalue(pieces, 565157600165888ULL) || hasvalue(pieces, 2256206466908160ULL) ||
//                       hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL))) ||
//               (index == 13 && (
//                       hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
//                       hasvalue(pieces, 2207646875648ULL) || hasvalue(pieces, 565157600165888ULL) ||
//                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL))) ||
//               (index == 14 && (
//                       hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
//                       hasvalue(pieces, 565157600165888ULL) || hasvalue(pieces, 567382628106240ULL))) ||
//               (index == 16 && (
//                       hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) ||
//                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 537921540ULL))) ||
//               (index == 17 && (
//                       hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) ||
//                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
//                       hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
//                       hasvalue(pieces, 16909320ULL))) ||
//               (index == 18 && (
//                       hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) ||
//                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
//                       hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 134480385ULL) ||
//                       hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
//                       hasvalue(pieces, 4328785920ULL) || hasvalue(pieces, 33818640ULL))) ||
//               (index == 19 && (
//                       hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
//                       hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 67372036ULL) ||
//                       hasvalue(pieces, 17247241216ULL) || hasvalue(pieces, 4415293751296ULL) ||
//                       hasvalue(pieces, 1130315200331776ULL) || hasvalue(pieces, 34426978560ULL) ||
//                       hasvalue(pieces, 17626613022720ULL) || hasvalue(pieces, 9024825867632640ULL) ||
//                       hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL) ||
//                       hasvalue(pieces, 67637280ULL))) ||
//               (index == 20 && (
//                       hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
//                       hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 17247241216ULL) ||
//                       hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 1130315200331776ULL) ||
//                       hasvalue(pieces, 8813306511360ULL) || hasvalue(pieces, 4512412933816320ULL) ||
//                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
//                       hasvalue(pieces, 17315143680ULL))) ||
//               (index == 21 && (
//                       hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
//                       hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 4415293751296ULL) ||
//                       hasvalue(pieces, 1130315200331776ULL) || hasvalue(pieces, 2256206466908160ULL) ||
//                       hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL))) ||
//               (index == 22 && (
//                       hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
//                       hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 1130315200331776ULL) ||
//                       hasvalue(pieces, 1134765256212480ULL))) ||
//               (index == 24 && (
//                       hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) ||
//                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 16909320ULL))) ||
//               (index == 25 && (
//                       hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) ||
//                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
//                       hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 4328785920ULL) ||
//                       hasvalue(pieces, 33818640ULL))) ||
//               (index == 26 && (
//                       hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) ||
//                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
//                       hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 268960770ULL) ||
//                       hasvalue(pieces, 137707914240ULL) || hasvalue(pieces, 1108169195520ULL) ||
//                       hasvalue(pieces, 8657571840ULL) || hasvalue(pieces, 67637280ULL))) ||
//               (index == 27 && (
//                       hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
//                       hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 134744072ULL) ||
//                       hasvalue(pieces, 34494482432ULL) || hasvalue(pieces, 8830587502592ULL) ||
//                       hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 134480385ULL) ||
//                       hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
//                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
//                       hasvalue(pieces, 17315143680ULL))) ||
//               (index == 28 && (
//                       hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
//                       hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 34494482432ULL) ||
//                       hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 2260630400663552ULL) ||
//                       hasvalue(pieces, 34426978560ULL) || hasvalue(pieces, 17626613022720ULL) ||
//                       hasvalue(pieces, 9024825867632640ULL) || hasvalue(pieces, 567382628106240ULL) ||
//                       hasvalue(pieces, 4432676782080ULL))) ||
//               (index == 29 && (
//                       hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
//                       hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 8830587502592ULL) ||
//                       hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 8813306511360ULL) ||
//                       hasvalue(pieces, 4512412933816320ULL) || hasvalue(pieces, 1134765256212480ULL))) ||
//               (index == 30 && (
//                       hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
//                       hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 2260630400663552ULL) ||
//                       hasvalue(pieces, 2256206466908160ULL))) ||
//               (index == 32 && (
//                       hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) || hasvalue(pieces, 269488144ULL) ||
//                       hasvalue(pieces, 33818640ULL))) ||
//               (index == 33 && (
//                       hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) || hasvalue(pieces, 269488144ULL) ||
//                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 8657571840ULL) ||
//                       hasvalue(pieces, 67637280ULL))) ||
//               (index == 34 && (
//                       hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) || hasvalue(pieces, 269488144ULL) ||
//                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
//                       hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 2216338391040ULL) ||
//                       hasvalue(pieces, 17315143680ULL))) ||
//               (index == 35 && (
//                       hasvalue(pieces, 503316480ULL) || hasvalue(pieces, 1006632960ULL) ||
//                       hasvalue(pieces, 269488144ULL) || hasvalue(pieces, 68988964864ULL) ||
//                       hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
//                       hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
//                       hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL))) ||
//               (index == 36 && (
//                       hasvalue(pieces, 128849018880ULL) || hasvalue(pieces, 257698037760ULL) ||
//                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
//                       hasvalue(pieces, 4521260801327104ULL) || hasvalue(pieces, 68853957120ULL) ||
//                       hasvalue(pieces, 35253226045440ULL) || hasvalue(pieces, 1134765256212480ULL))) ||
//               (index == 37 && (
//                       hasvalue(pieces, 32985348833280ULL) || hasvalue(pieces, 65970697666560ULL) ||
//                       hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
//                       hasvalue(pieces, 17626613022720ULL) || hasvalue(pieces, 9024825867632640ULL))) ||
//               (index == 38 && (
//                       hasvalue(pieces, 8444249301319680ULL) || hasvalue(pieces, 16888498602639360ULL) ||
//                       hasvalue(pieces, 4521260801327104ULL) || hasvalue(pieces, 4512412933816320ULL))) ||
//               (index == 40 && (
//                       hasvalue(pieces, 60ULL) || hasvalue(pieces, 538976288ULL) || hasvalue(pieces, 67637280ULL))) ||
//               (index == 41 && (
//                       hasvalue(pieces, 15360ULL) || hasvalue(pieces, 538976288ULL) ||
//                       hasvalue(pieces, 137977929728ULL) ||
//                       hasvalue(pieces, 17315143680ULL))) ||
//               (index == 42 && (
//                       hasvalue(pieces, 3932160ULL) || hasvalue(pieces, 538976288ULL) ||
//                       hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
//                       hasvalue(pieces, 4432676782080ULL))) ||
//               (index == 43 && (
//                       hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 538976288ULL) ||
//                       hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
//                       hasvalue(pieces, 9042521602654208ULL) || hasvalue(pieces, 537921540ULL) ||
//                       hasvalue(pieces, 1134765256212480ULL))) ||
//               (index == 44 && (
//                       hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 137977929728ULL) ||
//                       hasvalue(pieces, 35322350010368ULL) || hasvalue(pieces, 9042521602654208ULL) ||
//                       hasvalue(pieces, 137707914240ULL))) ||
//               (index == 45 && (
//                       hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 35322350010368ULL) ||
//                       hasvalue(pieces, 9042521602654208ULL) || hasvalue(pieces, 35253226045440ULL))) ||
//               (index == 46 && (
//                       hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 9042521602654208ULL) ||
//                       hasvalue(pieces, 9024825867632640ULL)));
    } else {
        switch (INDEX_LOOKUP_TABLE_ROW_COL[row][col]) {
//        switch (INDEX_LOOKUP_TABLE_COL_ROW[col][row]) {
            case 0:
                return hasvalue(pieces, 15ULL) || hasvalue(pieces, 16843009ULL) || hasvalue(pieces, 134480385ULL);
            case 1:
                return hasvalue(pieces, 3840ULL) || hasvalue(pieces, 16843009ULL) || hasvalue(pieces, 4311810304ULL) ||
                       hasvalue(pieces, 34426978560ULL);
            case 2:
                return hasvalue(pieces, 983040ULL) || hasvalue(pieces, 16843009ULL) ||
                       hasvalue(pieces, 4311810304ULL) || hasvalue(pieces, 1103823437824ULL) ||
                       hasvalue(pieces, 8813306511360ULL);
            case 3:
                return hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 16843009ULL) ||
                       hasvalue(pieces, 4311810304ULL) || hasvalue(pieces, 1103823437824ULL) ||
                       hasvalue(pieces, 282578800082944ULL) || hasvalue(pieces, 2256206466908160ULL) ||
                       hasvalue(pieces, 16909320ULL);
            case 4:
                return hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 4311810304ULL) ||
                       hasvalue(pieces, 1103823437824ULL) || hasvalue(pieces, 282578800082944ULL) ||
                       hasvalue(pieces, 4328785920ULL);
            case 5:
                return hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 1103823437824ULL) ||
                       hasvalue(pieces, 282578800082944ULL) || hasvalue(pieces, 1108169195520ULL);
            case 6:
                return hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 282578800082944ULL) ||
                       hasvalue(pieces, 283691314053120ULL);
            case 8:
                return hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 33686018ULL) ||
                       hasvalue(pieces, 268960770ULL);
            case 9:
                return hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 33686018ULL) ||
                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 134480385ULL) ||
                       hasvalue(pieces, 68853957120ULL);
            case 10:
                return hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 33686018ULL) ||
                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
                       hasvalue(pieces, 34426978560ULL) || hasvalue(pieces, 17626613022720ULL) ||
                       hasvalue(pieces, 16909320ULL);
            case 11:
                return hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                       hasvalue(pieces, 33686018ULL) || hasvalue(pieces, 8623620608ULL) ||
                       hasvalue(pieces, 2207646875648ULL) || hasvalue(pieces, 565157600165888ULL) ||
                       hasvalue(pieces, 8813306511360ULL) || hasvalue(pieces, 4512412933816320ULL) ||
                       hasvalue(pieces, 4328785920ULL) || hasvalue(pieces, 33818640ULL);
            case 12:
                return hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                       hasvalue(pieces, 8623620608ULL) || hasvalue(pieces, 2207646875648ULL) ||
                       hasvalue(pieces, 565157600165888ULL) || hasvalue(pieces, 2256206466908160ULL) ||
                       hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL);
            case 13:
                return hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                       hasvalue(pieces, 2207646875648ULL) || hasvalue(pieces, 565157600165888ULL) ||
                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL);
            case 14:
                return hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
                       hasvalue(pieces, 565157600165888ULL) || hasvalue(pieces, 567382628106240ULL);
            case 16:
                return hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) ||
                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 537921540ULL);
            case 17:
                return hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) ||
                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
                       hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
                       hasvalue(pieces, 16909320ULL);
            case 18:
                return hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) ||
                       hasvalue(pieces, 67372036ULL) || hasvalue(pieces, 17247241216ULL) ||
                       hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 134480385ULL) ||
                       hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
                       hasvalue(pieces, 4328785920ULL) || hasvalue(pieces, 33818640ULL);
            case 19:
                return hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                       hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 67372036ULL) ||
                       hasvalue(pieces, 17247241216ULL) || hasvalue(pieces, 4415293751296ULL) ||
                       hasvalue(pieces, 1130315200331776ULL) || hasvalue(pieces, 34426978560ULL) ||
                       hasvalue(pieces, 17626613022720ULL) || hasvalue(pieces, 9024825867632640ULL) ||
                       hasvalue(pieces, 1108169195520ULL) || hasvalue(pieces, 8657571840ULL) ||
                       hasvalue(pieces, 67637280ULL);
            case 20:
                return hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                       hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 17247241216ULL) ||
                       hasvalue(pieces, 4415293751296ULL) || hasvalue(pieces, 1130315200331776ULL) ||
                       hasvalue(pieces, 8813306511360ULL) || hasvalue(pieces, 4512412933816320ULL) ||
                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
                       hasvalue(pieces, 17315143680ULL);
            case 21:
                return hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                       hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 4415293751296ULL) ||
                       hasvalue(pieces, 1130315200331776ULL) || hasvalue(pieces, 2256206466908160ULL) ||
                       hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL);
            case 22:
                return hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
                       hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 1130315200331776ULL) ||
                       hasvalue(pieces, 1134765256212480ULL);
            case 24:
                return hasvalue(pieces, 15ULL) || hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) ||
                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 16909320ULL);
            case 25:
                return hasvalue(pieces, 3840ULL) || hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) ||
                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
                       hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 4328785920ULL) ||
                       hasvalue(pieces, 33818640ULL);
            case 26:
                return hasvalue(pieces, 983040ULL) || hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) ||
                       hasvalue(pieces, 134744072ULL) || hasvalue(pieces, 34494482432ULL) ||
                       hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 268960770ULL) ||
                       hasvalue(pieces, 137707914240ULL) || hasvalue(pieces, 1108169195520ULL) ||
                       hasvalue(pieces, 8657571840ULL) || hasvalue(pieces, 67637280ULL);
            case 27:
                return hasvalue(pieces, 251658240ULL) || hasvalue(pieces, 503316480ULL) ||
                       hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 134744072ULL) ||
                       hasvalue(pieces, 34494482432ULL) || hasvalue(pieces, 8830587502592ULL) ||
                       hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 134480385ULL) ||
                       hasvalue(pieces, 68853957120ULL) || hasvalue(pieces, 35253226045440ULL) ||
                       hasvalue(pieces, 283691314053120ULL) || hasvalue(pieces, 2216338391040ULL) ||
                       hasvalue(pieces, 17315143680ULL);
            case 28:
                return hasvalue(pieces, 64424509440ULL) || hasvalue(pieces, 128849018880ULL) ||
                       hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 34494482432ULL) ||
                       hasvalue(pieces, 8830587502592ULL) || hasvalue(pieces, 2260630400663552ULL) ||
                       hasvalue(pieces, 34426978560ULL) || hasvalue(pieces, 17626613022720ULL) ||
                       hasvalue(pieces, 9024825867632640ULL) || hasvalue(pieces, 567382628106240ULL) ||
                       hasvalue(pieces, 4432676782080ULL);
            case 29:
                return hasvalue(pieces, 16492674416640ULL) || hasvalue(pieces, 32985348833280ULL) ||
                       hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 8830587502592ULL) ||
                       hasvalue(pieces, 2260630400663552ULL) || hasvalue(pieces, 8813306511360ULL) ||
                       hasvalue(pieces, 4512412933816320ULL) || hasvalue(pieces, 1134765256212480ULL);
            case 30:
                return hasvalue(pieces, 4222124650659840ULL) || hasvalue(pieces, 8444249301319680ULL) ||
                       hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 2260630400663552ULL) ||
                       hasvalue(pieces, 2256206466908160ULL);
            case 32:
                return hasvalue(pieces, 30ULL) || hasvalue(pieces, 60ULL) || hasvalue(pieces, 269488144ULL) ||
                       hasvalue(pieces, 33818640ULL);
            case 33:
                return hasvalue(pieces, 7680ULL) || hasvalue(pieces, 15360ULL) || hasvalue(pieces, 269488144ULL) ||
                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 8657571840ULL) ||
                       hasvalue(pieces, 67637280ULL);
            case 34:
                return hasvalue(pieces, 1966080ULL) || hasvalue(pieces, 3932160ULL) || hasvalue(pieces, 269488144ULL) ||
                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
                       hasvalue(pieces, 537921540ULL) || hasvalue(pieces, 2216338391040ULL) ||
                       hasvalue(pieces, 17315143680ULL);
            case 35:
                return hasvalue(pieces, 503316480ULL) || hasvalue(pieces, 1006632960ULL) ||
                       hasvalue(pieces, 269488144ULL) || hasvalue(pieces, 68988964864ULL) ||
                       hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
                       hasvalue(pieces, 268960770ULL) || hasvalue(pieces, 137707914240ULL) ||
                       hasvalue(pieces, 567382628106240ULL) || hasvalue(pieces, 4432676782080ULL);
            case 36:
                return hasvalue(pieces, 128849018880ULL) || hasvalue(pieces, 257698037760ULL) ||
                       hasvalue(pieces, 68988964864ULL) || hasvalue(pieces, 17661175005184ULL) ||
                       hasvalue(pieces, 4521260801327104ULL) || hasvalue(pieces, 68853957120ULL) ||
                       hasvalue(pieces, 35253226045440ULL) || hasvalue(pieces, 1134765256212480ULL);
            case 37:
                return hasvalue(pieces, 32985348833280ULL) || hasvalue(pieces, 65970697666560ULL) ||
                       hasvalue(pieces, 17661175005184ULL) || hasvalue(pieces, 4521260801327104ULL) ||
                       hasvalue(pieces, 17626613022720ULL) || hasvalue(pieces, 9024825867632640ULL);
            case 38:
                return hasvalue(pieces, 8444249301319680ULL) || hasvalue(pieces, 16888498602639360ULL) ||
                       hasvalue(pieces, 4521260801327104ULL) || hasvalue(pieces, 4512412933816320ULL);
            case 40:
                return hasvalue(pieces, 60ULL) || hasvalue(pieces, 538976288ULL) || hasvalue(pieces, 67637280ULL);
            case 41:
                return hasvalue(pieces, 15360ULL) || hasvalue(pieces, 538976288ULL) ||
                       hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 17315143680ULL);
            case 42:
                return hasvalue(pieces, 3932160ULL) || hasvalue(pieces, 538976288ULL) ||
                       hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
                       hasvalue(pieces, 4432676782080ULL);
            case 43:
                return hasvalue(pieces, 1006632960ULL) || hasvalue(pieces, 538976288ULL) ||
                       hasvalue(pieces, 137977929728ULL) || hasvalue(pieces, 35322350010368ULL) ||
                       hasvalue(pieces, 9042521602654208ULL) || hasvalue(pieces, 537921540ULL) ||
                       hasvalue(pieces, 1134765256212480ULL);
            case 44:
                return hasvalue(pieces, 257698037760ULL) || hasvalue(pieces, 137977929728ULL) ||
                       hasvalue(pieces, 35322350010368ULL) || hasvalue(pieces, 9042521602654208ULL) ||
                       hasvalue(pieces, 137707914240ULL);
            case 45:
                return hasvalue(pieces, 65970697666560ULL) || hasvalue(pieces, 35322350010368ULL) ||
                       hasvalue(pieces, 9042521602654208ULL) || hasvalue(pieces, 35253226045440ULL);
            case 46:
                return hasvalue(pieces, 16888498602639360ULL) || hasvalue(pieces, 9042521602654208ULL) ||
                       hasvalue(pieces, 9024825867632640ULL);
//        default: {
//            cout << "error1" << endl;
//            exit(0);
//        }
        }
    }
}

#endif //CONNECT4_ALPHABETA_H
