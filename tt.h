#ifndef TT_H
#define TT_H

#include "defs.h"
#include "board.h"

// transposition table data structure
typedef struct{
    U64 hash_key;
    int depth;
    int flag;
    int score;
} tt;

extern int hash_entires;
extern tt *hash_table;

// function declarations
void clear_hash_table(void);
void init_hash_table(int mb);
int read_hash_entry(int alpha, int beta, int depth);
void write_hash_entry(int score, int depth, int hash_flag);

#endif // TT_H
