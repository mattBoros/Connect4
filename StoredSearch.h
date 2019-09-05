#ifndef CONNECT4_SEARCHRESULT_H
#define CONNECT4_SEARCHRESULT_H

#include "State.h"

class StoredSearch {
public:
    uint64_t blackPieces;
    uint64_t whitePieces;
    int8_t alpha;
    int8_t beta;

    inline explicit StoredSearch(
            const State node,
            const int8_t alpha,
            const int8_t beta
    ) :
            blackPieces(node.blackPieces),
            whitePieces(node.whitePieces),
            alpha(alpha),
            beta(beta) {}

    inline bool operator==(const StoredSearch other) const {
        return blackPieces == other.blackPieces &&
               whitePieces == other.whitePieces &&
               alpha == other.alpha &&
               beta == other.beta;
    }

    inline const StoredSearch& operator=(const StoredSearch other) noexcept {
        blackPieces = other.blackPieces;
        whitePieces = other.whitePieces;
        alpha = other.alpha;
        beta = other.beta;
        return *this;
    }

};

static const StoredSearch NULL_CACHE_KEY(
        State(UINT64_MAX, UINT64_MAX, UINT32_MAX, UINT32_MAX,
              0, 0, 0, 0, 0, 0, 0),
        INT8_MAX, INT8_MAX);

// TODO: have it reversible
//static inline uint64_t reverse_board(const uint64_t board){
//
//}


#endif //CONNECT4_SEARCHRESULT_H
