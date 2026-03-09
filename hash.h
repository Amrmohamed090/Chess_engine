#ifndef HASH_H
#define HASH_H

#include "defs.h"

// random hash keys
extern U64 piece_keys[12][64];
extern U64 enpassant_keys[64];
extern U64 castle_keys[16];
extern U64 side_key;

// function declarations
void init_hash_keys(void);
U64 generate_hash_key(void);

#endif // HASH_H
