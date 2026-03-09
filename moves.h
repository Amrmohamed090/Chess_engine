#ifndef MOVES_H
#define MOVES_H

#include "defs.h"
#include "board.h"
#include "attacks.h"
#include "hash.h"

// move encoding/decoding macros
#define encode_move(source, target, piece, promoted, capture, double, enpassant, casteling) \
((source)) | \
(((target)) << 6) | \
(((piece)) << 12) | \
(((promoted)) << 16) | \
(((capture)) << 20) | \
(((double)) << 21) | \
(((enpassant)) << 22) | \
(((casteling)) << 23)

#define get_move_source(move) (move & 0x3f)
#define get_move_target(move) ((move & 0xfC0)>>6)
#define get_move_piece(move) ((move & 0xf000) >>12)
#define get_move_promoted(move) ((move & 0xf0000) >>16)
#define get_move_capture(move) ((move & 0x100000))
#define get_move_double(move) ((move & 0x200000))
#define get_move_enpassant(move) ((move & 0x400000))
#define get_move_castle(move) ((move & 0x800000))

// board state copy/restore macros
#define copy_board() \
    U64 bitboards_copy[12], occupancies_copy[3]; \
    int side_copy, enpassant_copy, castle_copy; \
    memcpy(bitboards_copy, bitboards, 96); \
    memcpy(occupancies_copy, occupancies, 24); \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle; \
    U64 hash_key_copy = hash_key;

#define take_back() \
    memcpy(bitboards, bitboards_copy, 96); \
    memcpy(occupancies, occupancies_copy, 24); \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy; \
    hash_key = hash_key_copy;

// move list structure
typedef struct {
    int moves[256];
    int count;
} moves;

// inline add move
static inline void add_move(moves *move_list, int move){
    move_list->moves[move_list->count] = move;
    move_list->count++;
}

// function declarations
void print_move(int move);
void print_move_list(moves *move_list);
int make_move(int move, int move_flag);
void generate_moves(moves *move_list);
void print_attacked_squares(int side);

#endif // MOVES_H
