#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#define U64 unsigned long long

// sides to move (colors)
enum {white, black, both};
// bishob and rook
enum {rook, bishob};

// square enumeration
enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// castling enumeration
enum {wk = 1, wq = 2, bk = 4, bq = 8};

// encode pieces
enum {P, N, B, R, Q, K, p, n, b, r, q, k};

// move flags
enum {all_moves, only_captures};

// FEN debug positions
#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

// search constants
#define infinity 50000
#define mate_value 49000
#define mate_score 48000
#define max_ply 64

// TT constants
#define no_hash_entry 100000
#define hash_flag_exact 0
#define hash_flag_alpha 1
#define hash_flag_beta 2

#endif // DEFS_H
