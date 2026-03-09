#ifndef EVAL_H
#define EVAL_H

#include "defs.h"
#include "board.h"
#include "attacks.h"
#include "bitboard.h"

// score arrays
extern int material_score[12];
extern const int pawn_score[64];
extern const int knight_score[64];
extern const int bishop_score[64];
extern const int rook_score[64];
extern const int king_score[64];
extern const int mirror_score[128];

// evaluation mask arrays
extern U64 file_masks[64];
extern U64 rank_masks[64];
extern U64 isolated_masks[64];
extern U64 white_passed_masks[64];
extern U64 black_passed_masks[64];

// function declarations
void initialize_evaluation_masks(void);
int evaluate(void);

#endif // EVAL_H
