#ifndef RANDOM_H
#define RANDOM_H

#include "defs.h"

extern unsigned int random_state;

unsigned int get_random_U32_number(void);
U64 get_random_U64_numbers(void);
U64 generate_magic_number(void);

#endif // RANDOM_H
