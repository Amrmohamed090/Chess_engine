#ifndef PERFT_H
#define PERFT_H

#include "defs.h"
#include "moves.h"

extern U64 nodes;

void perft_driver(int depth);
void perft_test(int depth);

#endif // PERFT_H
