#ifndef BITBOARD_H
#define BITBOARD_H

#include "defs.h"

// bit macros
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// file mask constants
extern const U64 not_a_file;
extern const U64 not_h_file;
extern const U64 not_hg_file;
extern const U64 not_ab_file;

// relevant bits arrays
extern const int bishob_relevant_bits[64];
extern const int rook_relevant_bits[64];

// count bits within a bitboard
#ifdef _MSC_VER
    #include <intrin.h>

    static inline int count_bits(U64 bitboard) {
        return (int)__popcnt64(bitboard);
    }

    static inline int get_ls1b_index(U64 bitboard) {
        unsigned long index;
        if (_BitScanForward64(&index, bitboard)) {
            return (int)index;
        }
        return -1;
    }
#else
    static inline int count_bits(U64 bitboard) {
        return __builtin_popcountll(bitboard);
    }

    static inline int get_ls1b_index(U64 bitboard) {
        if (bitboard) {
            return __builtin_ctzll(bitboard);
        }
        return -1;
    }
#endif

// function declarations
void print_bitboard(U64 bitboard);

#endif // BITBOARD_H
