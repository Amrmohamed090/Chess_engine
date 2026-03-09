#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"
#include "moves.h"
#include "eval.h"
#include "tt.h"

extern int killer_moves[2][max_ply];
extern int history_moves[12][64];
extern int pv_length[max_ply];
extern int pv_table[max_ply][max_ply];
extern int follow_pv;
extern int score_pv;
extern U64 nodes;

// forward declaration of communicate() from uci.c
void communicate(void);

// function declarations
int score_move(int move);
void sort_moves(moves *move_list);
int is_repetition(void);
int quiescense(int alpha, int beta);
int negamax(int alpha, int beta, int depth);
void search_position(int depth);
void enable_pv_scoring(moves *move_list);

#endif // SEARCH_H
