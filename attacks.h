#ifndef ATTACKS_H
#define ATTACKS_H

#include "defs.h"
#include "bitboard.h"

// attack tables
extern U64 pawn_attacks[2][64];
extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 bishob_masks[64];
extern U64 rook_masks[64];
extern U64 bishob_attacks[64][512];
extern U64 rook_attacks[64][4096];
extern U64 rook_magic_numbers[64];
extern U64 bishob_magic_numbers[64];

// inline attack getters
static inline U64 get_bishob_attacks(int square, U64 occupancy){
    return bishob_attacks[square][((occupancy & bishob_masks[square]) * bishob_magic_numbers[square]) >> (64 - bishob_relevant_bits[square])];
}

static inline U64 get_rook_attacks(int square, U64 occupancy){
    return rook_attacks[square][((occupancy & rook_masks[square]) * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square])];
}

static inline U64 get_queen_attacks(int square, U64 occupancy){
    return get_bishob_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

// function declarations
U64 mask_pawn_attacks(int side, int square);
U64 mask_knight_attacks(int square);
U64 mask_king_attacks(int square);
U64 mask_bishob_attacks(int square);
U64 mask_rook_attacks(int square);
U64 bishob_attacks_on_the_fly(int square, U64 block);
U64 rook_attacks_on_the_fly(int square, U64 block);
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);
U64 find_magic_number(int square, int relevant_bits, int bishob);
void init_magic_numbers(void);
void init_leapers_attacks(void);
void init_sliders_attacks(int bishob);
int is_square_attacked(int square, int side);
void print_attacked_squares(int side);

#endif // ATTACKS_H
