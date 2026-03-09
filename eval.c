#include "eval.h"

// material score
int material_score[12] = {
    100,    // white pawn
    300,    // white knight
    315,    // white bishop
    500,    // white rook
    900,    // white queen
    10000,  // white king
    -100,   // black pawn
    -300,   // black knight
    -315,   // black bishop
    -500,   // black rook
    -900,   // black queen
    -10000, // black king
};

// pawn positional score
const int pawn_score[64] =
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] =
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

// bishop positional score
const int bishop_score[64] =
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0
};

// rook positional score
const int rook_score[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0
};

// king positional score
const int king_score[64] =
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

// mirror positional score tables for opposite side
const int mirror_score[128] =
{
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
};

// evaluation mask arrays
U64 file_masks[64];
U64 rank_masks[64];
U64 isolated_masks[64];
U64 white_passed_masks[64];
U64 black_passed_masks[64];

// extract rank from a square
static const int get_rank[64] =
{
    7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0
};

// evaluation penalties and bonuses
static const int double_pawn_penalty = -10;
static const int isolated_pawn_penalty = -10;
static const int passed_pawn_bonus[8] = { 0, 10, 30, 50, 75, 100, 150, 200 };
static const int semi_open_file_score = 10;
static const int open_file_score = 15;
static const int king_sheild_bonus = 5;

// set file or rank mask
static U64 set_file_rank_mask(int file_number, int rank_number){
    U64 mask = 0ULL;
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            if (file_number != -1){
                if (file == file_number){
                    set_bit(mask, square);
                }
            }
            else if (rank_number != -1){
                if (rank == rank_number){
                    set_bit(mask, square);
                }
            }
        }
    }
    return mask;
}

void initialize_evaluation_masks(){
    // initialize file masks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            file_masks[square] |= set_file_rank_mask(file, -1);
        }
    }

    // initialize rank masks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            rank_masks[square] |= set_file_rank_mask(-1, rank);
        }
    }

    // initialize isolated masks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            isolated_masks[square] |= set_file_rank_mask(file -1, -1);
            isolated_masks[square] |= set_file_rank_mask(file +1, -1);
        }
    }

    // initialize white passed pawn masks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            white_passed_masks[square] |= set_file_rank_mask(file -1, -1);
            white_passed_masks[square] |= set_file_rank_mask(file, -1);
            white_passed_masks[square] |= set_file_rank_mask(file +1, -1);
            for (int i = 0; i < (8-rank); i++){
                white_passed_masks[square] &= ~rank_masks[(7-i)* 8 + file];
            }
        }
    }

    // initialize black passed pawn masks
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file<8; file++){
            int square = rank * 8 + file;
            black_passed_masks[square] |= set_file_rank_mask(file -1, -1);
            black_passed_masks[square] |= set_file_rank_mask(file, -1);
            black_passed_masks[square] |= set_file_rank_mask(file +1, -1);
            for (int i = 0; i < rank +1; i++){
                black_passed_masks[square] &= ~rank_masks[i* 8 + file];
            }
        }
    }
}

int evaluate(){
    int score = 0;
    U64 bitboard;
    int square;

    for (int bb_piece = P; bb_piece <= k; bb_piece++){
        bitboard = bitboards[bb_piece];
        int double_pawns = 0;

        while (bitboard){
            square = get_ls1b_index(bitboard);
            score += material_score[bb_piece];

            switch (bb_piece){
                case P :
                    score += pawn_score[square];
                    double_pawns = count_bits(bitboards[P] & file_masks[square]);
                    if (double_pawns > 1)
                        score += double_pawns * double_pawn_penalty;
                    if ((bitboards[P] & isolated_masks[square]) == 0)
                        score += isolated_pawn_penalty;
                    if ((white_passed_masks[square] & bitboards[p]) == 0)
                        score += passed_pawn_bonus[get_rank[square]];
                    break;
                case N :
                    score += knight_score[square];
                    break;
                case B :
                    score += bishop_score[square];
                    score += count_bits(get_bishob_attacks(square, occupancies[both]));
                    break;
                case R :
                    score += rook_score[square];
                    if ((bitboards[P] & file_masks[square]) == 0)
                        score += semi_open_file_score;
                    if ((bitboards[P] & file_masks[square]) == 0 && (bitboards[p] & file_masks[square]) == 0)
                        score += open_file_score;
                    break;
                case Q :
                    score += count_bits(get_queen_attacks(square, occupancies[both]));
                    break;
                case K :
                    score += king_score[square];
                    if ((bitboards[r] | bitboards[q]) != 0){
                        if ((bitboards[P] & file_masks[square]) == 0)
                            score -= semi_open_file_score;
                        if ((bitboards[P] & file_masks[square]) == 0 && (bitboards[p] & file_masks[square]) == 0)
                            score -= open_file_score;
                    }
                    score += count_bits(king_attacks[square] & occupancies[white]) * king_sheild_bonus;
                    break;
                case p :
                    score -= pawn_score[mirror_score[square]];
                    double_pawns = count_bits(bitboards[p] & file_masks[square]);
                    if (double_pawns > 1){
                        score -= double_pawns * double_pawn_penalty;
                    }
                    if ((bitboards[p] & isolated_masks[square]) == 0)
                        score -= isolated_pawn_penalty;
                    if ((black_passed_masks[square] & bitboards[P]) == 0)
                        score -= passed_pawn_bonus[get_rank[mirror_score[square]]];
                    break;
                case n : score -= knight_score[mirror_score[square]]; break;
                case b :
                    score -= bishop_score[mirror_score[square]];
                    score -= count_bits(get_bishob_attacks(square, occupancies[both]));
                    break;
                case r :
                    score -= rook_score[mirror_score[square]];
                    if ((bitboards[p] & file_masks[square]) == 0)
                        score -= semi_open_file_score;
                    if ((bitboards[p] & file_masks[square]) == 0 && (bitboards[P] & file_masks[square]) == 0)
                        score -= open_file_score;
                    break;
                case q :
                    score -= count_bits(get_queen_attacks(square, occupancies[both]));
                    break;
                case k :
                    score -= king_score[mirror_score[square]];
                    if ((bitboards[R] | bitboards[Q]) != 0){
                        if ((bitboards[p] & file_masks[square]) == 0)
                            score += semi_open_file_score;
                        if ((bitboards[p] & file_masks[square]) == 0 && (bitboards[P] & file_masks[square]) == 0)
                            score += open_file_score;
                    }
                    score -= count_bits(king_attacks[square] & occupancies[black]) * king_sheild_bonus;
                    break;
            }
            pop_bit(bitboard, square);
        }
    }

    return (side == white) ? score : -score;
}
