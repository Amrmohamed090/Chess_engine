#include "attacks.h"
#include "board.h"
#include "random.h"

// attack tables
U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 bishob_masks[64];
U64 rook_masks[64];
U64 bishob_attacks[64][512];
U64 rook_attacks[64][4096];

U64 mask_pawn_attacks(int side, int square){
    U64 attacks = 0ULL;
    U64 bitboard = 1ULL << square;

    if (!side){
        if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    }
    else{
        if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    }
    return attacks;
}

U64 mask_knight_attacks(int square){
    U64 attacks = 0ULL;
    U64 bitboard = 1ULL << square;

    if ((bitboard >> 17) & not_h_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_a_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_hg_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_ab_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_a_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_h_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_ab_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_hg_file) attacks |= (bitboard << 6);

    return attacks;
}

U64 mask_king_attacks(int square){
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    if ((bitboard >> 8)) attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & not_h_file) attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & not_a_file) attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & not_h_file) attacks |= (bitboard >> 1);
    if ((bitboard << 8)) attacks |= (bitboard << 8);
    if ((bitboard << 9) & not_a_file) attacks |= (bitboard << 9);
    if ((bitboard << 7) & not_h_file) attacks |= (bitboard << 7);
    if ((bitboard << 1) & not_a_file) attacks |= (bitboard << 1);

    return attacks;
}

U64 mask_bishob_attacks(int square){
    U64 attacks = 0ULL;
    int r, f;
    int tr = square >> 3;
    int tf = square & 7;

    for (r= tr+1, f=tf+1; r <=6 && f<=6; r++, f++) attacks |= (1ULL << (r * 8 + f));
    for (r= tr-1, f=tf+1; r >=1 && f<=6; r--, f++) attacks |= (1ULL << (r * 8 + f));
    for (r= tr+1, f=tf-1; r <=6 && f>=1; r++, f--) attacks |= (1ULL << (r * 8 + f));
    for (r= tr-1, f=tf-1; r >=1 && f>=1; r--, f--) attacks |= (1ULL << (r * 8 + f));

    return attacks;
}

U64 mask_rook_attacks(int square){
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r= tr+1; r <=6 ; r++) attacks |= (1ULL << (r * 8 + tf));
    for (r= tr-1; r >=1 ; r--) attacks |= (1ULL << (r * 8 + tf));
    for (f= tf+1; f <=6 ; f++) attacks |= (1ULL << (tr * 8 + f));
    for (f= tf-1; f >=1 ; f--) attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

U64 bishob_attacks_on_the_fly(int square, U64 block){
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r= tr+1, f=tf+1; r <=7 && f<=7; r++, f++){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r= tr-1, f=tf+1; r >=0 && f<=7; r--, f++){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r= tr+1, f=tf-1; r <=7 && f>=0; r++, f--){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }
    for (r= tr-1, f=tf-1; r >=0 && f>=0; r--, f--){
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block) break;
    }

    return attacks;
}

U64 rook_attacks_on_the_fly(int square, U64 block){
    U64 attacks = 0ULL;
    int r, f;
    int tr = square / 8;
    int tf = square % 8;

    for (r= tr+1; r <=7; r++){
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL <<  (r * 8 + tf)) & block) break;
    }
    for (r= tr-1; r >=0; r--){
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL <<  (r * 8 + tf)) & block) break;
    }
    for (f= tf+1; f <=7; f++){
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL <<  (tr * 8 + f)) & block) break;
    }
    for (f= tf-1; f >=0; f--){
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL <<  (tr * 8 + f)) & block) break;
    }

    return attacks;
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask){
    U64 occupancy = 0ULL;
    for (int count = 0; count < bits_in_mask; count++){
        int square = get_ls1b_index(attack_mask);
        pop_bit(attack_mask, square);
        if (index & (1 << count)) occupancy |= (1ULL << square);
    }
    return occupancy;
}

U64 find_magic_number(int square, int relevant_bits, int bishob){
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 used_attacks[4096];

    U64 attack_mask = bishob ? mask_bishob_attacks(square) : mask_rook_attacks(square);
    int occupancy_indicies = 1 << relevant_bits;

    for (int index = 0; index < occupancy_indicies; index++){
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
        attacks[index] = bishob ? bishob_attacks_on_the_fly(square, occupancies[index]) :
                                    rook_attacks_on_the_fly(square, occupancies[index]);
    }

    for (int random_count = 0; random_count < 100000000; random_count++){
        U64 magic_number = generate_magic_number();
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;
        memset(used_attacks, 0ULL, sizeof(used_attacks));
        int index, fail;
        for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++){
            int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));
            if (used_attacks[magic_index] == 0ULL)
                used_attacks[magic_index] = attacks[index];
            else if (used_attacks[magic_index] != attacks[index])
                fail = 1;
        }
        if (!fail)
            return magic_number;
    }

    printf("  Magic number fails!\n");
    return 0ULL;
}

void init_magic_numbers(){
    for (int square = 0; square < 64; square++){
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    }
    for (int square = 0; square < 64; square++){
        bishob_magic_numbers[square] = find_magic_number(square, bishob_relevant_bits[square], bishob);
    }
}

void init_leapers_attacks(){
    for (int square = 0; square < 64; square ++){
        pawn_attacks[white][square] = mask_pawn_attacks(white, square);
        pawn_attacks[black][square] = mask_pawn_attacks(black, square);
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
    }
}

void init_sliders_attacks(int bishob){
    for (int square = 0; square < 64; square ++){
        bishob_masks[square] = mask_bishob_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishob ? bishob_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indicies = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indicies; index ++){
            if(bishob){
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = occupancy * bishob_magic_numbers[square] >> (64 - bishob_relevant_bits[square]);
                bishob_attacks[square][magic_index] = bishob_attacks_on_the_fly(square, occupancy);
            }
            else{
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = occupancy * rook_magic_numbers[square] >> (64 - rook_relevant_bits[square]);
                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}

int is_square_attacked(int square, int side){
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;
    if (get_bishob_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;
    return 0;
}

void print_attacked_squares(int side){
    for (int rank = 0; rank < 8; rank++){
        for (int file = 0; file < 8; file ++){
            int square = 8*rank + file;
            if (!file)
                printf(" %d", 8 - rank);
            printf(" %d", is_square_attacked(square, side) ? 1: 0);
        }
        printf("\n");
    }
    printf("\n   a b c d e f g h\n\n");
}
