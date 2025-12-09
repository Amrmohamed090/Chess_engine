

#include <stdio.h>

#define U64 unsigned long long


enum { white, black};
enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1
};


const char* coordinates_to_square[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

/******************************\
================================
        Bit Manipulation
================================
\******************************/
// bit macros

// set/get/pop
#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (bitboard &= ~((1ULL) << square));

// count bits within a bitboard
// to-do: find a better implementation of counting bits,
// some engines have bmi versions of it, they make use of hardware bit manipulation instructions in a hardware level
static inline int count_bits(U64 bitboard){

    // bit counter

    int count = 0;
    // return bit count

    while (bitboard)
    {
        // increment count
        count ++;

        // increment count
        bitboard &= bitboard -1;
    }

    return count;
}

// get least significant 1st bit index

static inline int get_ls1b_index(U64 bitboard){
    
    // make sure bitboard is not zero
    if (bitboard){
        // count trailing bis before LS1B

        return count_bits((bitboard & -bitboard) - 1);



    }
    else return -1;
}

/******************************\
================================
        Input and output
================================
\******************************/
//print bitboard
void print_bitboard(U64 bitboard){
    // loop over board ranks


    for (int rank = 0; rank < 8; rank ++){
        // loop over board files
        printf("\n");

        for (int file = 0; file < 8; file ++){

            // convert file and rank into square index

            int square = rank * 8 + file;
            //print ranks

            if (!file) printf(" %d ", 8 - rank);
            // print bit state, either 1 or 0
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }

        // print board files

    }
    printf("\n\n    a b c d e f g h\n\n");
    printf("    Bitboard: %llud\n\n", bitboard);
}


/******************************\
================================
            Attacks
================================
\******************************/

// pawn attacks table


// pawn attacks table [side][square]
U64 pawn_attacks[2][64];

/*
    not A file

 8  0 1 1 1 1 1 1 1
 7  0 1 1 1 1 1 1 1
 6  0 1 1 1 1 1 1 1
 5  0 1 1 1 1 1 1 1
 4  0 1 1 1 1 1 1 1
 3  0 1 1 1 1 1 1 1
 2  0 1 1 1 1 1 1 1
 1  0 1 1 1 1 1 1 1

    a b c d e f g h
*/

// not A file constant
const U64 not_a_file = 18374403900871474942ULL;
const U64 not_h_file = 9187201950435737471ULL;
const U64 not_hg_file = 4557430888798830399ULL;
const U64 not_ab_file = 18229723555195321596ULL;

// generate pawn attacks [side] [square]
U64 pawn_attacks[2][64];

//knight attacks table [square]
U64 knight_attacks[64];

//king attacks table [square]
U64 king_attacks[64];

U64 mask_pawn_attacks(int side, int square){
    // result attacks bitboard;
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // white pawns

    if (!side){
        if ((bitboard >> 7) & not_a_file)attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file)attacks |= (bitboard >> 9);

    }
    else{
        if ((bitboard << 7) & not_h_file)attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file)attacks |= (bitboard << 9);

    }
    return attacks;
}

// generate knight attack

U64 mask_knight_attacks(int square){
    // result attacks bitboard;
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    //generate knight attacks
    if ((bitboard >> 17) & not_h_file)
    attacks |= (bitboard >> 17);
    
    if ((bitboard >> 15) & not_a_file)
    attacks |= (bitboard >> 15);


    if ((bitboard >> 10) & not_hg_file)
    attacks |= (bitboard >> 10);

    if ((bitboard >> 6) & not_ab_file)
    attacks |= (bitboard >> 6);


    //generate knight attacks
    if ((bitboard << 17) & not_a_file)
    attacks |= (bitboard << 17);
    
    if ((bitboard << 15) & not_h_file)
    attacks |= (bitboard << 15);


    if ((bitboard << 10) & not_ab_file)
    attacks |= (bitboard << 10);

    if ((bitboard << 6) & not_hg_file)
    attacks |= (bitboard << 6);




    return attacks;
}

U64 mask_king_attacks(int square){
    // result attacks bitboard;
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    if ((bitboard >> 8)) attacks |= (bitboard >> 8);
    
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);

    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);

    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);


    // opposit direction
    if ((bitboard << 8)) attacks |= (bitboard << 8);
    
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);

    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);

    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

    return attacks;

}

// mask bishop's attacks

U64 mask_bishob_attacks(int square){
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target ranks & files
    int tr = square / 8;
    int tf = square % 8;
    
    // mask relevant bishop occupancy bits
    for (r= tr+1, f=tf+1; r <=6 && f<=6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r= tr-1, f=tf+1; r >=1 && f<=6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r= tr+1, f=tf-1; r <=6 && f>=1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r= tr-1, f=tf-1; r >=1 && f>=1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}

// mask rook attacks
U64 mask_rook_attacks(int square){
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target ranks & files
    int tr = square / 8;
    int tf = square % 8;
    
    // mask relevant bishop occupancy bits
    for (r= tr+1; r <=6 ; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r= tr-1; r >=1 ; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f= tf+1; f <=6 ; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f= tf-1; f >=1 ; f--) attacks |= (1ULL << (tr * 8 + f));


    return attacks;
}

// generate bishob attacks on the fly
U64 bishob_attacks_on_the_fly(int square, U64 block){
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target ranks & files
    int tr = square / 8;
    int tf = square % 8;
    
    // generate bishob attacks
    for (r= tr+1, f=tf+1; r <=7 && f<=7; r++, f++) 
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    
    }
    for (r= tr-1, f=tf+1; r >=0 && f<=7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;

    }
    for (r= tr+1, f=tf-1; r <=7 && f>=0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;

    }
    for (r= tr-1, f=tf-1; r >=0 && f>=0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;

    }

    return attacks;
}


U64 rook_attacks_on_the_fly(int square, U64 block){
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target ranks & files
    int tr = square / 8;
    int tf = square % 8;
    
    // mask relevant bishop occupancy bits
    for (r= tr+1; r <=7; r++) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL <<  (r * 8 + tf)) & block) break;
    }
    for (r= tr-1; r >=0; r--) {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL <<  (r * 8 + tf)) & block) break;

    }
    for (f= tf+1; f <=7; f++) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL <<  (tr * 8 + f)) & block) break;

    }
    for (f= tf-1; f >=0; f--) {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL <<  (tr * 8 + f)) & block) break;

    }


    return attacks;
}



// init leaper pieces attacks

void init_leapers_attacks(){

    // loop over 64 board squares

    for (int square = 0; square < 64; square ++){

        // init pawn attacks
        pawn_attacks[white][square] = mask_pawn_attacks(white, square);
        pawn_attacks[black][square] = mask_pawn_attacks(black, square);

        // init knight attacks

        knight_attacks[square] = mask_knight_attacks(square);

        // init king attack

        king_attacks[square] = mask_king_attacks(square);
    }
}


U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask){
    // occupancy map
    U64 occupancy = 0ULL;

    // loop over the range of bits within attack mask
    for (int count = 0; count < bits_in_mask; count++){
        // get LS1B index of attacks mask

        int square = get_ls1b_index(attack_mask);

        // pop LS1B in attack map
        pop_bit(attack_mask, square);

        // make sure occupancy is on board
        if (index & (1 << count)) occupancy |= (1ULL << square);
    }

    //return occupancy map
    return occupancy;
}

int main(){

    // init leaper piece attacks

    init_leapers_attacks();


    // mask piece attacks at given square
    U64 attack_mask = mask_rook_attacks(a1);

    // loop over occupancy indicies
    for (int i = 0; i < 4095; i++){
        U64 occupancy = set_occupancy(i, count_bits(attack_mask), attack_mask);

        print_bitboard(occupancy);}

    // for (int square = 0; square < 64; square ++)
    // print_bitboard(bishob_attacks_on_the_fly(square, 0ULL));
    // printf("bit count: %d\n", count_bits(block));
    return 0;
}