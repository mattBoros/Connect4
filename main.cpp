
#include <iostream>
#include <fstream>

using namespace std;

#include "AlphaBeta.h"
#include "State.h"
#include "Piece.h"

int main(int argc, char *argv[]) {
    cout << "started" << endl;
//    test123();
//    exit(0);

//    test_score_func();
//    exit(0);

//    uint64_t blackPieces = _atoi64(argv[1]);
//    uint64_t whitePieces = _atoi64(argv[2]);
//    unsigned char depth = atol(argv[3]);
//    unsigned int side = atol(argv[4]);

//    const bool sideBool = sideBool ? Piece::BLACK : Piece::WHITE;
//    if(side == 1){
//        sideBool = Piece::BLACK;
//    } else {
//        sideBool = Piece::WHITE;
//    }
    const bool sideBool = Piece::BLACK;
//    uint64_t blackPieces = 289510328633712640ULL;
//    uint64_t whitePieces = 4672591690525580290ULL;
    const State board = Helpers2::getStartingBoard();
    uint64_t reversed = reverse_board(board.whitePieces);
//    const State board = Util::getBoard(
//            "__WWW__"
//            "WBWBWBW"
//            "BWBWBWB"
//            "BWBWBWB"
//            "WBWBWBW"
//            "WBWBWBW"
//            );
//    Util::print_ull(15ULL);
//    cout << ((int) alphaBeta.min_value(board, -100, 100, 0)) << endl;
    cout << "initializing alphabeta" << endl;
    // 34 :
    // 35 :
    // 36 :
    // 37 :
    // 38 : 326
    // 39 :
    const AlphaBeta<38, sideBool> alphaBeta;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int t1 = tp.tv_sec * 1000 + tp.tv_usec / 1000;

//    cout << (int) alphaBeta.pvs_init<sideBool>(board) << endl;
    const uint8_t move = alphaBeta.MTDF(board);
    cout << "move to make : " << (int) move << endl;
//    cout << (int) alphaBeta.pvs_no_cache<sideBool,0>(board, NEG_INFINITY, POS_INFINITY) << endl;

    gettimeofday(&tp, NULL);
    long int t2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    double time = (t2 - t1) / 1000.0;


    cout << "Time taken : " << time << endl;

//    cout << "Apply Time : " << TIME::applyTime / 1000.0 << endl;
//    cout << "Action Get Time : " << TIME::getActionTime / 1000.0 << endl;
//    cout << "BS Set Time : " << TIME::bsSetTime / 1000.0 << endl;
//    cout << "BS Get Time : " << TIME::bsGetTime / 1000.0 << endl;
    cout << "Positions searched              : " << TIME::positions_searched << endl;
    cout << "Positions per second            : " << TIME::positions_searched / time << endl;
    cout << "Cutoffs                         : " << TIME::cutoffs << endl;
    cout << "Non-Cutoffs                     : " << TIME::non_cutoffs << endl;
    cout << "Num wrong searches              : " << TIME::num_wrong_searches << endl;
    cout << "Num max height ifs              : " << TIME::num_max_height_ifs << endl;
    cout << "Num max below max height ifs    : " << TIME::num_below_max_height_ifs << endl;
    cout << "ttable hits                      : " << TIME::cache_hits << endl;
    cout << "ttable misses                    : " << TIME::cache_misses << endl;
    cout << "num last depths                 : " << TIME::num_last_depths << endl;
    cout << "num forced moves == 0           : " << TIME::num_fm_zero << endl;
    cout << "num forced moves == 1           : " << TIME::num_fm_one << endl;
    cout << "num forced moves >= 2           : " << TIME::num_fm_two << endl;
    cout << "num short circuit wins          : " << TIME::num_short_circuit_wins << endl;

//    getchar();
}



