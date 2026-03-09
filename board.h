#ifndef BOARD_H
#define BOARD_H

#include "defs.h"

// piece bitboards
extern U64 bitboards[12];

// occupancies bitboards
extern U64 occupancies[3];

// side to move
extern int side;

// enpassant square
extern int enpassant;

// castling rights
extern int castle;

// hash key
extern U64 hash_key;

// positions repetition table
extern U64 repetitions_table[1000];

// repetition index
extern int repetition_index;

// half move counter
extern int ply;

// piece representation arrays
extern char ascii_pieces[12];
extern char *unicode_pieces[12];
extern int char_pieces[];
extern char promoted_pieces[];
extern const char* square_to_coordinates[];
extern const int castling_rights[64];

// function declarations
void parse_fen(char* fen);
void print_board(void);

#endif // BOARD_H
