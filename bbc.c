/*************************\
===========================
Credit: 
Base Code forked by Code Monkey

Several Improvement are implemented, and more left to implement

Done:
Use Hardware instructions for popbit and get least significant bit
ToDo

implementation of fancy magic bitboards

===========================
\************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
#endif


#define U64 unsigned long long

// sides to move (colors)
enum {white, black, both};
// bishob and rook
enum {rook, bishob};

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "


const int bishob_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};

const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};


// rook magic numbers
U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};

// bishob magic numbers
U64 bishob_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};


const char* square_to_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "-"
};

/**********************************\
 ==================================
 
            Chess board
 
 ==================================
\**********************************/


/* WHITE PIECES
                            WHITE PIECES


        Pawns                  Knights              bishobs
        
  8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  1 1 1 1 1 1 1 1    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 1 0 0 0 0 1 0    1  0 0 1 0 0 1 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


         Rooks                 Queens                 King

  8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  1 0 0 0 0 0 0 1    1  0 0 0 1 0 0 0 0    1  0 0 0 0 1 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


                            BLACK PIECES


        Pawns                  Knights              bishobs
        
  8  0 0 0 0 0 0 0 0    8  0 1 0 0 0 0 1 0    8  0 0 1 0 0 1 0 0
  7  1 1 1 1 1 1 1 1    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


         Rooks                 Queens                 King

  8  1 0 0 0 0 0 0 1    8  0 0 0 1 0 0 0 0    8  0 0 0 0 1 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h



                             OCCUPANCIES


     White occupancy       Black occupancy       All occupancies

  8  0 0 0 0 0 0 0 0    8  1 1 1 1 1 1 1 1    8  1 1 1 1 1 1 1 1
  7  0 0 0 0 0 0 0 0    7  1 1 1 1 1 1 1 1    7  1 1 1 1 1 1 1 1
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  1 1 1 1 1 1 1 1    2  0 0 0 0 0 0 0 0    2  1 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1    1  0 0 0 0 0 0 0 0    1  1 1 1 1 1 1 1 1



                            ALL TOGETHER

                        8  ♜ ♞ ♝ ♛ ♚ ♝ ♞ ♜
                        7  ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎
                        6  . . . . . . . .
                        5  . . . . . . . .
                        4  . . . . . . . .
                        3  . . . . . . . .
                        2  ♙ ♙ ♙ ♙ ♙ ♙ ♙ ♙
                        1  ♖ ♘ ♗ ♕ ♔ ♗ ♘ ♖

                           a b c d e f g h

*/


// castling rights binary encoding

/*

    bin  dec
    
   0001    1  white king can castle to the king side
   0010    2  white king can castle to the queen side
   0100    4  black king can castle to the king side
   1000    8  black king can castle to the queen side

   examples

   1111       both sides an castle both directions
   1001       black king => queen side
              white king => king side

*/


// castling rights update constants

/*
                           castling   move     in      in
                              right update     binary  decimal

 king & rooks didn't move:     1111 & 1111  =  1111    15

        white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13
     
         black king moved:     1111 & 0011  =  1011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7

*/

// castling rights update constants
const int castling_rights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};



//casteling enumeration
enum {wk = 1, wq = 2, bk = 4, bq = 8};

// encode pieces
enum {P, N, B, R, Q, K, p, n, b, r, q, k};


// ASCII pieces
char ascii_pieces[12] = "PNBRQKpnbrqk";

// unicode pieces
char *unicode_pieces[12] = {"♟︎","♞","♝","♜","♛","♚","♙","♘","♗","♖","♕","♔"};

// convert ascii pieces character to encoded contants
int char_pieces[] = {
['P'] = P,
['N'] = N,
['B'] = B,
['R'] = R,
['Q'] = Q,
['K'] = K,
['p'] = p,
['n'] = n,
['b'] = b,
['r'] = r,
['q'] = q,
['k'] = k};

// promoted pieces
char promoted_pieces[]={
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N]= 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n]= 'n'
};

// piece bitboards
U64 bitboards[12];

// occupancies bitboards;
U64 occupancies[3];

// side to move
int side = -1;

// enpassant square;
int enpassant = no_sq;


// castling rights
int castle;

/********************************\
==================================
      Time control variables
==================================
\********************************/

// exit from engine flag
int quit = 0;

// UCI "movetime" command time counter
int movestogo = 30;

// UCI "inc" command's time increment holder
int time = -1;

// UCI "starttime" command time holder
int starttime  0;

//

unsigned int random_state = 1804289383;
//generate 32-bit psedu legal numbers
unsigned int get_random_U32_number(){

    unsigned int number = random_state;

    // Xor shift algorithm

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    random_state = number;

    return number;

}



/******************************\
================================
        Random numbers
================================
\******************************/

// generate 64 bit pseudo legal numbers

U64 get_random_U64_numbers(){

    // define 4 random numbers

    U64 n1, n2, n3, n4;

    // init random numbers

    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;

    n3 = (U64)(get_random_U32_number()) & 0xFFFF;

    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    return n1 | (n2 << 16) |(n3 <<32)| (n4 << 48);

}

// generate magic number candidate

U64 generate_magic_number(){

    return get_random_U64_numbers() & get_random_U64_numbers() & get_random_U64_numbers();
}

/******************************\
================================
        Bit Manipulation
================================
\******************************/
// bit macros

// set/get/pop
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))


// count bits within a bitboard
// to-do: find a better implementation of counting bits,
// some engines have bmi versions of it, they make use of hardware bit manipulation instructions in a hardware level
// MSVC (Windows)
#ifdef _MSC_VER
    #include <intrin.h>
    
    static inline int count_bits(U64 bitboard) {
        return (int)__popcnt64(bitboard);
    }

    static inline int get_ls1b_index(U64 bitboard) {
        unsigned long index;
        // _BitScanForward64 returns 0 if mask is zero, non-zero otherwise
        if (_BitScanForward64(&index, bitboard)) {
            return (int)index;
        }
        return -1;
    }

// GCC / Clang (Linux, Mac, MinGW)
#else
    static inline int count_bits(U64 bitboard) {
        return __builtin_popcountll(bitboard);
    }

    static inline int get_ls1b_index(U64 bitboard) {
        if (bitboard) {
            // __builtin_ctzll = Count Trailing Zeros
            return __builtin_ctzll(bitboard);
        }
        return -1;
    }
#endif

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

void print_board(){
    for (int rank = 0; rank<8; rank++){

        for ( int file = 0; file < 8; file ++){

            int square = rank * 8 + file;
            if (!file)
            printf(" %d", 8-rank);
            // define piece variable
            int piece = -1;

            // loop over all bitboards and ask if there is a bit over this square or not
            for (int bb_piece = P; bb_piece <= k; bb_piece++){
                if (get_bit(bitboards[bb_piece], square))
                    piece = bb_piece;
            }

            // print different piece set depending on operating system
            #ifdef WIN64
            printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
            #else
            printf(" %s", ((piece == -1) ? "." : unicode_pieces[piece]));
            #endif
            // print chess board
        }
        // print new line every rank
        printf("\n");
        
    }
    printf("\n    a b c d e f g h\n\n");

    printf("    Side:      %s\n", (!side) ? "w" : "b" );
    printf("    Enpassant: %s\n", (enpassant != no_sq) ? square_to_coordinates[enpassant] : "no");
    printf("    Casteling: %c%c%c%c\n\n", (castle & wk) ? 'K': '-',
                                            (castle & wq) ? 'Q': '-',
                                            (castle & bk) ? 'k': '-',
                                            (castle & bq) ? 'q': '-');

}


void parse_fen(char* fen){

    // reset board position (bitboards)
    memset(bitboards, 0ULL, sizeof(bitboards));

    // resent occupancies (bitboards)
    memset(occupancies, 0ULL, sizeof(occupancies));
    
    //reset game state variable
    side = 0;
    enpassant = no_sq;
    castle = 0;

    // loop over board ranks

    for (int rank = 0; rank<8; rank++){
        // loop over board files
        for ( int file = 0; file < 8; file++){
            int square = rank * 8 + file;

            // match assci pieces within FEN string
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')){
                int piece = char_pieces[*fen];

                // set piece on corresponding bitboard

                set_bit(bitboards[piece], square);

                // increment pointer to FEN string

                fen++;
            }
            if (*fen >= '0' && *fen <= '9'){
                // init offset (convert char 0 to int 0)
                int offset = *fen - '0';
                int piece = -1;

                for (int bb_piece = P; bb_piece <= k; bb_piece++){
                    // if there is a pice on current square
                    if (get_bit(bitboards[bb_piece], square))
                        // get piece code
                        piece = bb_piece;
                }

                // on empty current square
                if (piece == -1){
                    // decrement the file
                    file --;
                }
                // adjust file counter
                file += offset;

                //increment pointer to fin
                fen++;
            }
            if (*fen =='/')fen++;
        }

    }
    fen++;

    // parse side to move
    (*fen == 'w') ? (side = white) : (side = black);

    fen += 2;

    // parse casteling rights
    while(*fen != ' '){
        switch(*fen){
            case 'K': castle |= wk; break;
            case 'Q': castle |= wq; break;
            case 'k': castle |= bk; break;
            case 'q': castle |= bq; break;
            case '.': break;

        }
        fen++;
    }
    fen++;

    if (*fen != ' ' && *fen != '-'){ 
        // parse enpassant file & rank
        int file = fen[0] - 'a';
        int rank = 8-(fen[1] - '0');

        enpassant = rank * 8 + file;
    }
    // no enpassant square
    else enpassant = no_sq;

    // loop over white pieces bitboards
    for (int piece = P; piece <= K; piece++){
        // populate white bitboard pieces
        occupancies[white] |= bitboards[piece];
    }

    // loop over black pieces bitboards
    for (int piece = p; piece <= k; piece++){
        // populate white bitboard pieces
        occupancies[black] |= bitboards[piece];
    }

    // init all occupancies

    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];
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

U64 bishob_masks[64];

U64 rook_masks[64];

U64 bishob_attacks[64][512];

U64 rook_attacks[64][4096];

U64 mask_pawn_attacks(int side, int square){
    // result attacks bitboard;
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 1ULL << square;

    // white pawns

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

// generate knight attack

U64 mask_knight_attacks(int square){
    // result attacks bitboard;
    U64 attacks = 0ULL;

    // piece bitboard
    // set piece on board

    U64 bitboard = 0ULL << square;

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

// mask bishob's attacks

U64 mask_bishob_attacks(int square){
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target ranks & files
    int tr = square / 8;
    int tf = square % 8;
    
    // mask relevant bishob occupancy bits
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
    
    // mask relevant bishob occupancy bits
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
    
    // mask relevant bishob occupancy bits
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

/******************************\
================================
        Magics
================================
\******************************/


// find appropriate magic number
U64 find_magic_number(int square, int relevant_bits, int bishob)
{
    // init occupancies
    U64 occupancies[4096];
    
    // init attack tables
    U64 attacks[4096];
    
    // init used attacks
    U64 used_attacks[4096];
    
    // init attack mask for a current piece
    U64 attack_mask = bishob ? mask_bishob_attacks(square) : mask_rook_attacks(square);
    
    // init occupancy indicies
    int occupancy_indicies = 1 << relevant_bits;
    
    // loop over occupancy indicies
    for (int index = 0; index < occupancy_indicies; index++)
    {
        // init occupancies
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
        
        // init attacks
        attacks[index] = bishob ? bishob_attacks_on_the_fly(square, occupancies[index]) :
                                    rook_attacks_on_the_fly(square, occupancies[index]);
    }
    
    // test magic numbers loop
    for (int random_count = 0; random_count < 100000000; random_count++)
    {
        // generate magic number candidate
        U64 magic_number = generate_magic_number();
        
        // skip inappropriate magic numbers
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;
        
        // init used attacks
        memset(used_attacks, 0ULL, sizeof(used_attacks));
        
        // init index & fail flag
        int index, fail;
        
        // test magic index loop
        for (index = 0, fail = 0; !fail && index < occupancy_indicies; index++)
        {
            // init magic index
            int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));
            
            // if magic index works
            if (used_attacks[magic_index] == 0ULL)
                // init used attacks
                used_attacks[magic_index] = attacks[index];
            
            // otherwise
            else if (used_attacks[magic_index] != attacks[index])
                // magic index doesn't work
                fail = 1;
        }
        
        // if magic number works
        if (!fail)
            // return it
            return magic_number;
    }
    
    // if magic number doesn't work
    printf("  Magic number fails!\n");
    return 0ULL;
}



// init magic numbers
void init_magic_numbers(){

    for (int square = 0; square < 64; square++){
        // init rook magic numbers
    rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    }

    for (int square = 0; square < 64; square++){
        // init rook magic numbers
        bishob_magic_numbers[square] = find_magic_number(square, bishob_relevant_bits[square], bishob);
    }


}


// init slide piece's attack tables

void init_sliders_attacks(int bishob){

    // loop over 64 board squares

    for (int square = 0; square < 64; square ++){

        // init bishob & rook masks
        bishob_masks[square] = mask_bishob_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        // init current mask
        U64 attack_mask = bishob ? bishob_masks[square] : rook_masks[square];
        
        // init relevant occupancy bit count
        int relevant_bits_count = count_bits(attack_mask);
        
        // init occupancy indicies
        int occupancy_indicies = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indicies; index ++){

            // bishob
            if(bishob){

                // init current occupancy variation
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = occupancy * bishob_magic_numbers[square] >> (64 - bishob_relevant_bits[square]);
                
                // init bishob attacks

                bishob_attacks[square][magic_index] = bishob_attacks_on_the_fly(square, occupancy);  

            }
            else{
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = occupancy * rook_magic_numbers[square] >> (64 - rook_relevant_bits[square]);
                
                // init bishob attacks

                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);  


            }
        }
    }
    // init bishob & rook masks


}

// Fancy magic bitboards - no masking required
static inline U64 get_bishob_attacks(int square, U64 occupancy){
    // get bishob attacks assuming current board occupancy (fancy magic)
    return bishob_attacks[square][((occupancy & bishob_masks[square]) * bishob_magic_numbers[square]) >> (64 - bishob_relevant_bits[square])];
}

// Fancy magic bitboards - no masking required
static inline U64 get_rook_attacks(int square, U64 occupancy){
    // get rook attacks assuming current board occupancy (fancy magic)
    return rook_attacks[square][((occupancy & rook_masks[square]) * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square])];
}

static inline U64 get_queen_attacks(int square, U64 occupancy){
    return get_bishob_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
}

// static inline U64 get_queen_attacks(int square, U64 occupancy){
//     // init result attacks bitboard
//     U64 queen_attacks = 0ULL;
    

//     U64 bishob_occupancy = occupancy;
//     U64 rook_occupancy  = occupancy;
//     bishob_occupancy &= bishob_masks[square];
//     bishob_occupancy *= bishob_magic_numbers[square];
//     bishob_occupancy >>= 64 - bishob_relevant_bits[square];


//     // get bishob attacks
//     queen_attacks = bishob_attacks[square][bishob_occupancy];

//     // get rook attacks assuming current board occupancy
//     rook_occupancy &= rook_masks[square];
//     rook_occupancy *= rook_magic_numbers[square];
//     rook_occupancy >>= 64 - rook_relevant_bits[square];
//     // return rook attacks
    
//     queen_attacks |= rook_attacks[square][rook_occupancy];
//     return queen_attacks;

// }




// is square current given square attacked by current given side
static inline int is_square_attacked(int square, int side){
    
    // attacked by white pawn
    if ((side == white) && (pawn_attacks[black][square] & bitboards[P])) return 1;
    // attacked by black pawn
    if ((side == black) && (pawn_attacks[white][square] & bitboards[p])) return 1;
    // attacked by knight
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;
    // attacked by king
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;
    //attacked by bishob
    if (get_bishob_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;
    // by default return False
    return 0;
}


/* 
NOTE : we are using only 24 bits, we can encode more information to the int
    binary move bits representation                   hexa_decimal      
    0000 0000 0000 0000 0011 1111   source square       0x3f
    0000 0000 0000 1111 1100 0000   target square       0xfC0
    0000 0000 1111 0000 0000 0000   piece               0xf000
    0000 1111 0000 0000 0000 0000   promoted piece      0xf0000
    0001 0000 0000 0000 0000 0000   capture flag        0x100000
    0010 0000 0000 0000 0000 0000   double push flag    0x200000
    0100 0000 0000 0000 0000 0000   enpassant flag      0x400000
    1000 0000 0000 0000 0000 0000   castling flag       0x800000
    */

// encode move macro
#define encode_move(source, target, piece, promoted, capture, double, enpassant, casteling) \
(source) | \
(target << 6) |\
(piece << 12) |\
(promoted << 16) |\
(capture << 20) | \
(double << 21)|\
(enpassant << 22)|\
(casteling << 23)

// extract source square
#define get_move_source(move) (move & 0x3f)

// extract target square
#define get_move_target(move) ((move & 0xfC0)>>6)

// extract move piece
#define get_move_piece(move) ((move & 0xf000) >>12)

// extract promoted piece
#define get_move_promoted(move) ((move & 0xf0000) >>16)

// extract capture flag
#define get_move_capture(move) ((move & 0x100000))

// extract push flag
#define get_move_double(move) ((move & 0x200000))

// extract enpassat flag
#define get_move_enpassant(move) ((move & 0x400000))

// extract casteling flag
#define get_move_castle(move) ((move & 0x800000))

// move list structure
typedef struct {
    int moves[256]; //moves 
    int count; //move count
} moves;

// add move to move list

static inline void add_move(moves *move_list, int move){
    move_list->moves[move_list->count] = move;
    move_list->count ++;
}

// print move 
void print_move(int move)
{
    if (get_move_promoted(move))
        printf("%s%s%c", square_to_coordinates[get_move_source(move)],
                           square_to_coordinates[get_move_target(move)],
                           promoted_pieces[get_move_promoted(move)]);
    else
        printf("%s%s", square_to_coordinates[get_move_source(move)],
                           square_to_coordinates[get_move_target(move)]);
}


void print_move_list(moves *move_list){
    if (!move_list->count){
        printf("\nno move in the move list\n");
        return;
    }
    printf("\ntotal number of moves: %d", move_list->count);
     printf("\nmove     piece    capture   double    enpassant    casteling \n");

    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        int move = move_list->moves[move_count];

        printf("%s%s%c    %c        %d         %d         %d           %d  \n", square_to_coordinates[get_move_source(move)],
                                            square_to_coordinates[get_move_target(move)],
                                            (get_move_promoted(move)? promoted_pieces[get_move_promoted(move)] : ' '),
                                            ascii_pieces[get_move_piece(move)],
                                            (get_move_capture(move)? 1: 0),
                                            (get_move_double(move)? 1: 0),
                                            (get_move_enpassant(move)? 1: 0),
                                            (get_move_castle(move) )? 1: 0);
    }
}



#define copy_board()\
    U64 bitboards_copy[12], occupancies_copy[3];                       \
    int side_copy, enpassant_copy, castle_copy;                        \
    memcpy(bitboards_copy, bitboards, 96);                             \
    memcpy(occupancies_copy, occupancies, 24);                         \
    side_copy = side, enpassant_copy = enpassant, castle_copy = castle;\

#define take_back()\
    memcpy(bitboards, bitboards_copy, 96);                             \
    memcpy(occupancies, occupancies_copy, 24);                         \
    side = side_copy, enpassant = enpassant_copy, castle = castle_copy;\



/**************************\
       Move Generator
\**************************/

enum {all_moves, only_captures};

static inline int make_move(int move, int move_flag){
    // quite moves
    if (move_flag == all_moves){
        // preserve board state

        copy_board(); // if this board expose the king with check, we will need to take it back
        
        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castle(move);


        // move piece
        pop_bit(bitboards[piece], source_square);
        set_bit(bitboards[piece], target_square);
        

        //updating occupancies
        pop_bit(occupancies[side], source_square);
        set_bit(occupancies[side], target_square);

        pop_bit(occupancies[both], source_square);
        set_bit(occupancies[both], target_square);

        // if there is an occupancies on the target square on the other side
        if (get_bit(occupancies[(side == white ? black: white)], target_square)){
            pop_bit(occupancies[(side == white ? black: white)], target_square);
        }


        // handling capture moves
        if (capture){
            // pick up bitboard piece index depending on side
            int start_piece , end_piece;

            // white to move
            if (side == white){
                start_piece = p;
                end_piece = k;
            }
            else{
                start_piece = P;
                end_piece = K;
            }

            // loop over bitboards oppiste to the current side to move
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
                // idea here is simple, if white is capturing something we need to take it out from the other side bitboards
                // if there is a piece on the target square, remove it from the correspondent bitboard
                if (get_bit(bitboards[bb_piece], target_square)){
                    pop_bit(bitboards[bb_piece], target_square);
                    break;
                }
            }
        }
        // handle pawn promotions
        if (promoted_piece){
            // we need to erase that pawn from target square
            pop_bit(bitboards[(side == white ? P:p)], target_square);
            // set up promoted piece on chess board
            set_bit(bitboards[promoted_piece], target_square);

        }

        // handle enpassant captures
        if (enpass){
            // erase the pawn depeding on side to move
            if (side == white){
                pop_bit(bitboards[p], target_square + 8);
                pop_bit(occupancies[black], target_square + 8);
                pop_bit(occupancies[both], target_square + 8);

            }
            else{
                pop_bit(bitboards[P], target_square - 8);
                pop_bit(occupancies[white], target_square - 8);
                pop_bit(occupancies[both], target_square - 8);

            }

        }
        // reset enpassant square
        enpassant = no_sq;

        if (double_push){
            (side == white) ? (enpassant = target_square + 8):(enpassant = target_square - 8);
        }

        if (castling){
            // handle castling move
            switch(target_square){
                // white castle king side
                case (g1):
                    // move H rook
                    pop_bit(bitboards[R], h1);
                    set_bit(bitboards[R], f1);

                    // move the rook in the occupancies for white and both
                    pop_bit(occupancies[white], h1);
                    set_bit(occupancies[white], f1);

                    pop_bit(occupancies[both], h1);
                    set_bit(occupancies[both], f1);


                    break;
                
                // white castle queen side
                case (c1):
                    // move a-file rook
                    pop_bit(bitboards[R], a1);
                    set_bit(bitboards[R], d1);
                    // move the rook in the occupancies of white
                    pop_bit(occupancies[white], a1);
                    set_bit(occupancies[white], d1);
                    // move the rook in the occupancies of both
                    pop_bit(occupancies[both], a1);
                    set_bit(occupancies[both], d1);
                    break;
                // black castle king side
                case (g8):
                    // move H rook
                    pop_bit(bitboards[r], h8);
                    set_bit(bitboards[r], f8);
                    // move the rook in the occupancies of black
                    pop_bit(occupancies[black], h8);
                    set_bit(occupancies[black], f8);
                    // move the rook in the occupancies of both
                    pop_bit(occupancies[both], h8);
                    set_bit(occupancies[both], f8);
                    break;

                // black castle queen side
                case (c8):
                    // move a8 rook
                    pop_bit(bitboards[r], a8);
                    set_bit(bitboards[r], d8);
                    // move the rook in the occupancies of black
                    pop_bit(occupancies[black], a8);
                    set_bit(occupancies[black], d8);
                    // move the rook in the occupancies of both
                    pop_bit(occupancies[both], a8);
                    set_bit(occupancies[both], d8);
                    break;
            }
        }
    // update castling rights
    castle &= castling_rights[source_square];
    castle &= castling_rights[target_square];
    
    // todo : I think updating the occupancies could be made more efficiently
    // we can create a macro that toggles both the bitboards[piece] and occupancies
    // reset occupancies
    // memset(occupancies, 0ULL, 24);
    // // loop over white pieces bitboards
    // for (int bb_piece = P; bb_piece <=K; bb_piece++)
    // occupancies[white] |= bitboards[bb_piece];

    // for (int bb_piece = b; bb_piece <=k; bb_piece++)
    // occupancies[black] |= bitboards[bb_piece];

    // // update both sides occupancies
    // occupancies[both] |= occupancies[white];
    // occupancies[both] |= occupancies[black];
    
    // change side
    side ^= 1;

    // make sure that king has not been exposed into a check
    if (is_square_attacked((side == white)? get_ls1b_index(bitboards[k]): get_ls1b_index(bitboards[K]), side))
        {// move is illegal, hence take it back
        take_back();
        // return illegal move
        return 0;
        }
    else return 1;

    // test occupancies
    U64 test_occupancies[3] = {0ULL, 0ULL, 0ULL};

    for (int bb_piece = P; bb_piece <=K; bb_piece++)
    test_occupancies[white] |= bitboards[bb_piece];

    for (int bb_piece = p; bb_piece <=k; bb_piece++)
    test_occupancies[black] |= bitboards[bb_piece];


    test_occupancies[both] |= test_occupancies[white];
    test_occupancies[both] |= test_occupancies[black];

    // update both sides occupancies

    
    if (test_occupancies[white] != occupancies[white]) printf("WARING: test failed in white occupancies\n");
    else printf("PASSED: test for white occupancies\n");

    if (test_occupancies[black] != occupancies[black]) printf("WARING: test failed in black occupancies\n");
    else printf("PASSED: test for black occupancies\n");
    
    if (test_occupancies[both] != occupancies[both]) printf("WARING: test failed in both occupancies\n");
    else printf("PASSED: test for both occupancies\n");

    }   
    // capture moves
    else{
        // make sure move is the capture
        if (get_move_capture(move))
            make_move(move, all_moves);
        else
            // don't make it
            return 0;

    }

}
//generate all moves
static inline void generate_moves(moves *move_list){
    // init move count
    move_list->count = 0;
    // define source & target squares;
    int source_square, target_square;

    // define current pieces's bitboard copy & it's attacks
    U64 bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++){
        // init piece bitboard copy
        bitboard = bitboards[piece];
        // generate white pawns a white king castling moves
        if (side == white){
            // pick up white pawn bitboards
            if (piece == P){

                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square - 8;

                    // generate quite pawn moves
                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square)){
                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7){
                            // add move into a move list
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));

                        }
                        else{                       
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            if((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square-8))
                            {
                                add_move(move_list, encode_move(source_square, target_square-8, piece, 0, 0, 1, 0, 0));
                            }

                        }
                    }
                    //init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    // generate pawn captures
                    while (attacks){
                        // init target square
                        target_square = get_ls1b_index(attacks);
                        // pawn capture
                        if (source_square >= a7 && source_square <= h7){
                            // add move into a move list
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));

                        }
                        else{                       
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        }
                        // pop ls1b
                        pop_bit(attacks, target_square);


                    }
                    // generate enpassant captures
                    if (enpassant != no_sq){
                        // lookup pawn attacks and bitwise AND with enpassant bit
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        // make sure enpassant capture available
                        if (enpassant_attacks){
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));

                        }
                    }
                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            //castling moves
            if (piece == K){
                // king side castling is avaliable

                if (castle & wk){
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1)){
                        
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }

                }
                // queen side castling is available
                if (castle & wq){
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], b1)){
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))

                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                }       }
            }
        }
        // generate black pawns a white king castling moves
        else{
            // pick up white pawn bitboards
            if (piece == p){

                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square + 8;

                    // generate quite pawn moves
                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square)){
                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2){
                            // add move into a move list
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else{         
                            // single pawn push              
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            if((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square+8))
                            // double pawn push
                            add_move(move_list, encode_move(source_square, target_square+8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    //init pawn attacks bitboard
                    attacks = pawn_attacks[side][source_square] & occupancies[white];

                    // generate pawn captures
                    while (attacks){
                        // init target square
                        target_square = get_ls1b_index(attacks);
                        // pawn capture
                        if (source_square >= a2 && source_square <= h2){
                            // add move into a move list
                            // pawn capture promotion
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }
                        else{                       
                            // pawn capture
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        // pop ls1b
                        pop_bit(attacks, target_square);


                    }
                    // generate enpassant captures
                    if (enpassant != no_sq){
                        // lookup pawn attacks and bitwise AND with enpassant bit
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);

                        // make sure enpassant capture available
                        if (enpassant_attacks){
                            // init enpassant capture target square
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));

                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }

            }
            //castling moves
            if (piece == k){
                // king side castling is avaliable
                
                if (castle & bk){
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8)){

                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                        
                    }

                }
                // queen side castling is available
                if (castle & bq){
                    // make sure square between king and king's rook are empty
                    if (!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], b8)){

                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white) && !is_square_attacked(c8, white))
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));                        
                    }

                }
            }      
                



        }
        // generate knight moves
        if ((side == white) ? piece == N: piece == n){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                // init piece attacks in order to get set of target squares
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    

                    // quite moves
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));                        
                    // caputre move
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));                        
                
                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }
                

                
                //pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }
        // generate bishob moves
        if ((side == white) ? piece == B: piece == b){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                // init piece attacks in order to get set of target squares
                attacks = get_bishob_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);;

                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    

                    // quite moves
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));                        
                    // caputre move
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));                        


                    

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }
                

                
                //pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R: piece == r){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                // init piece attacks in order to get set of target squares
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);;

                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    

                    // quite moves
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));                        
                    // caputre move
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));                        


                    

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }
                

                
                //pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q: piece == q){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                // init piece attacks in order to get set of target squares
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);;

                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    

                    // quite moves
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));                        
                    // caputre move
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));                        
                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }
                

                
                //pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K: piece == k){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                // init piece attacks in order to get set of target squares
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    // quite moves
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));                        
                    // caputre move
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));                        
                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }
                

                
                //pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

    }


}



// print attacked squares
void print_attacked_squares(int side){
    // loop over board ranks
    for (int rank = 0; rank < 8; rank++){
        //loop over files
        for (int file = 0; file < 8; file ++){
            // init square
            int square = 8*rank + file;
            //print ranks 
            if (!file)
                printf(" %d", 8 - rank);

            // check whether the square is attacked or not 
            printf(" %d", is_square_attacked(square, side) ? 1: 0);

            
            // print new line every rank
        }
        printf("\n");

    }
    printf("\n   a b c d e f g h\n\n");
}

/******************************\
================================
        main driver
================================
\******************************/

int get_time_ms(){
    #ifdef WIN64
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif

}
// int test_suit(char * test_fen){
//     init_all();
//     parse_fen(test_fen);
//     moves move_list[1];

//     //generate moves list
//     generate_moves(move_list);
//     int test_count = 0;

//     for (int move_count = 0; move_count < move_list->count; move_count++){
//         int move = move_list->moves[move_count];
//         copy_board();

//         if (!make_move(move,all_moves))
//             continue;
//         // print_board();
//         // getchar();
//         test_count ++;
//         take_back();
//         // print_board();
//         // getchar();

//     }
//     return test_count;
// }

// leaf nodes (number of position reached during the test of the move generator at a given depth)
long nodes = 0;
// perft driver

static inline void perft_driver(int depth){

    // recursion escape condition
    if (depth == 0){
        // increment nodes count (count reached positions)
        nodes++;
        return;
    }



    moves move_list[1];
    //generate moves list
    generate_moves(move_list);

    for (int move_count = 0; move_count < move_list->count; move_count++){
        int move = move_list->moves[move_count];
        copy_board();
        if (!make_move(move,all_moves))
            continue;

        // call perft driver recursivly
        perft_driver(depth - 1);
        take_back();

    }

}

void perft_test(int depth){
    printf("    Performance Test \n\n");
    moves move_list[1];
    //generate moves list
    generate_moves(move_list);

    // initialize start time
    long start_time = get_time_ms();
    for (int move_count = 0; move_count < move_list->count; move_count++){
        int move = move_list->moves[move_count];
        copy_board();
        if (!make_move(move,all_moves))
            continue;

        long cumulative_nodes = nodes;
        perft_driver(depth - 1);
        long old_nodes = nodes - cumulative_nodes;
        // call perft driver recursivly
        printf("    move: %s%s%c  nodes: %ld \n", 
                        square_to_coordinates[get_move_source(move)],
                        square_to_coordinates[get_move_target(move)],
                        promoted_pieces[get_move_promoted(move)]
                        , old_nodes);
        take_back();

        }

        //print results summary
        printf("\n    Depth: %d\n", depth);
        printf("    Nodes: %d\n", nodes);
        printf("    Time:  %ldms\n", get_time_ms()-start_time);
}

/******************************\
================================
              Evaluation
================================
\******************************/
// material score
int material_score[12] = {
    100,    // white pawn
    300,    // white knight
    315,    // white bishop
    500,    // white rook
    900,    // white queen
    10000,  // white king
    -100,   // black pawn
    -300,   // black knight
    -315,   // black bishop
    -500,   // black rook
    -900,   // black queen
    -10000, // black king
};


// pawn positional score
const int pawn_score[64] = 
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

// knight positional score
const int knight_score[64] = 
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

// bishop positional score
const int bishop_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

// rook positional score
const int rook_score[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

// king positional score
const int king_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

// mirror positional score tables for opposite side
const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

// position evaluation
static inline int evaluate(){
    // static evaluation score
    int score = 0;
    
    // current pieces bitboard copy
    U64 bitboard;

    // init piece & square
    int piece, square;

    // loop over pieces bitboards

    for (int bb_piece = P; bb_piece <= k; bb_piece++){
        // init piece bitboard copy

        bitboard = bitboards[bb_piece];

        // loop over pieces within a bitbaord
        while (bitboard){
            square = get_ls1b_index(bitboard);
            // score material weights
            score += material_score[bb_piece];

            switch (bb_piece){
                //evaluate white pieces positions
                case P : score += pawn_score[square]; break;
                case N : score += knight_score[square]; break;
                case B : score += bishop_score[square]; break;
                case R : score += rook_score[square]; break; 
                case K : score += king_score[square]; break;

                //evaluate black pieces positions
                case p : score -= pawn_score[mirror_score[square]]; break;
                case n : score -= knight_score[mirror_score[square]]; break;
                case b : score -= bishop_score[mirror_score[square]]; break;
                case r : score -= rook_score[mirror_score[square]]; break;
                case k : score -= king_score[mirror_score[square]]; break;

            }
            //pop ls1b
            pop_bit(bitboard, square);

        }
    }
    // return final evaluation

     return (side == white) ? score : -score;


}



/******************************\
================================
              search
================================
\******************************/

// most valuable victim & less valuable attacher

/*
                          
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// MVV LVA [attacker][victim]
static int mvv_lva[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};


// max ply that we can reach within a search
#define max_ply 64

// killer moves [id][ply]
// https://www.chessprogramming.org/Killer_Move
int killer_moves[2][max_ply];



// history moves [piece][square]
int history_moves[12][64];

// half move counter
int ply;




/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5
      
      0    m1   m2   m3   m4   m5   m6
      
      1    0    m2   m3   m4   m5   m6 
      
      2    0    0    m3   m4   m5   m6
      
      3    0    0    0    m4   m5   m6
       
      4    0    0    0    0    m5   m6
      
      5    0    0    0    0    0    m6
*/

// PV length
int pv_length[max_ply];

// PV table [ply][ply]
int pv_table[max_ply][max_ply];
// follow pv = 1: we follow the principle variation from previous iterative deepinging pass, pv=0 we do not follow it
int follow_pv, score_pv;

// half move counter
int ply;


//enable pv scoring
static inline void enable_pv_scoring(moves* move_list){
    // disable following pv
    follow_pv = 0;
    // loop over the moves within a move list
    for (int count = 0; count < move_list->count; count++){

        // make sure we hit PV move
        if (pv_table[0][ply] == move_list->moves[count]){
            // enable move scoring
            score_pv = 1;

            // enable following pv 
            follow_pv = 1;
        }
    }
}

// score moves for move ordering
static inline int score_move(int move){


    // if pv move scoring is allowed

    if (score_pv){
        if (pv_table[0][ply] == move){

            // disable score PV flag
            score_pv = 0;
            
            // give PV move the highest score to search it first
            return 20000;
        }
    }
    // todo IMPORTANT // to-do: you can optimize this part by encoding the target piece to the move U64 int

    // score caputre move
    if (get_move_capture(move)){

        // initialize to a pawn, in case of enpassant capture, there will be no piece on this square, so it will always be a pawn, it does not matter if it is a black pawn or a white pawn, the lookup table will return the same value
        int target_piece = p;

        int target_square = get_move_target(move);
        // pick up bitboard piece index depending on side
        int start_piece , end_piece;

        // white to move
        if (side == white){ start_piece = p; end_piece = k;}
        else{ start_piece = P; end_piece = K;}

        // loop over bitboards oppiste to the current side to move
        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
            if (get_bit(bitboards[bb_piece], target_square)){
                target_piece = bb_piece;
                break;
            }
        }

        // score move by MVV LVA lookup [source piece][target piece]
        
        // printf("source piece: %c \n", ascii_pieces[get_move_piece(move)]);
        // printf("target piece: %c \n\n", ascii_pieces[target_piece]);
        return mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }
    // score quiet move
    else{
        // score 1st killer move
        if (killer_moves[0][ply] == move)
            return 9000;
        // score 2nd killer move
        else if (killer_moves[1][ply] == move)
            return 8000;
        
        // score history move
        else
            return history_moves[get_move_piece(move)][get_move_target(move)];
    }
    return 0;
}

/***************\
    Sorting 
\***************/

// TODO: you can implement a function called get_best_move, instead of sorting the entire array, you can just make a single pass to the moves and get the maximum score at each move order, 
// Insertion sort - best for small lists and partially ordered data
// Sorts moves in descending order by score (highest score first)
void sort_moves(moves* move_list, int count) {
    int i, j;
    int temp_move;
    int temp_score;
    
    for (i = 1; i < count; i++) {
        temp_move = move_list->moves[i];
        temp_score = score_move(temp_move);
        j = i - 1;
        
        // Shift elements that are smaller than temp_score to the right
        while (j >= 0 && score_move(move_list->moves[j]) < temp_score) {
            move_list->moves[j + 1] = move_list->moves[j];
            j--;
        }
        
        move_list->moves[j + 1] = temp_move;
    }
}


static inline int quiescense (int alpha, int beta){
    
    // incrementnodes count
    nodes++;

    
    // evaluate position
    int evaluation = evaluate();
    // fail hard beta cutoff 1- fail hard frame work, score cant be outside of alpha beta bounds 2- fail soft
    if (evaluation >= beta){
        // nodes fails high
        return beta;
    }

    // found a better move
    if (evaluation > alpha){
        // PV node (move) 
        alpha = evaluation;

    }

    // create move list instance
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);

    sort_moves(move_list, move_list->count);
    // loop over moves within a movelist

    for (int count = 0; count<move_list->count; count++){
        // preserve board state
        copy_board();
        
        // increment ply
        ply++;

        // make sure to make only legal moves
        if (make_move(move_list->moves[count], only_captures) == 0){
            // decrement ply
            ply--;
            
            // skip to the next move
            continue;
        }

        // score current move
        int score = -quiescense(-beta, -alpha);

        // decrement ply
        ply--;

        // take move back
        take_back();

        // fail hard beta cutoff 1- fail hard frame work, score cant be outside of alpha beta bounds 2- fail soft
        if (score >= beta){
            // nodes fails high
            return beta;
        }

        // found a better move
        if (score > alpha){
            // PV node (move) 
            alpha = score;

        }
    }

    // node (move) fails low

    return alpha;
}

const int full_depth_moves = 4;
const int reduction_limit = 3;


// negamax alpha beta
static inline int negamax(int alpha, int beta, int depth){
    

    // init PV length
    pv_length[ply] = ply;

    // recursive escapre condition
    if (depth == 0){
        return quiescense(alpha, beta);
    }
    // increment nodes count
    nodes++;

    //we are too deep hence there's an overflow of array relying on max ply constant
    if (ply > max_ply - 1){
        // evaluate position
        return evaluate();
    }


    // is king in check
    int in_check = is_square_attacked((side == white) ? get_ls1b_index(bitboards[K]):  get_ls1b_index(bitboards[k]), side ^ 1);
    
    // increase search depth if the king has been exposed into a check
    if (in_check) depth++;


    // legal moves counter
    int legal_moves = 0;

    if (depth >= 3 && in_check == 0 && ply){
        copy_board();

        // switch the side, literally giving opponent an extra move to make
        side ^= 1;

        enpassant = no_sq;

        // search moves with reduced depth to find the beta cutoff

        int score = -negamax( -beta, -beta + 1,depth - 1 - 2);
        
        // restore board state
        take_back();

        // fail-hard beta cuttoff
        if (score >= beta)return beta;
    }
    // old value of alpha
    int old_alpha = alpha;
    
    // create move list instance
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // if we are now following pv line

    if (follow_pv)
        // enable pv move score
        enable_pv_scoring(move_list);
    // sort moves
    sort_moves(move_list, move_list->count);
    // loop over moves within a movelist

    
    // moves_searched
    int moves_searched = 0;

    for (int count = 0; count<move_list->count; count++){
        // preserve board state
        copy_board();
        
        // increment ply
        ply++;

        // make sure to make only legal moves
        if (make_move(move_list->moves[count], all_moves) == 0){
            // decrement ply
            ply--;
            
            // skip to the next move
            continue;
        }

        // increment legal moves counter
        legal_moves++;

        // varialbe to store current move score from the static evaluation perspective
        int score;


        if (moves_searched == 0)
            // do normal search
            score = -negamax(-beta, -alpha, depth -1);
        // late move reduction 
        else{
            if (moves_searched >= full_depth_moves
                && depth >= reduction_limit 
                && in_check == 0 
                && get_move_capture(move_list->moves[count]) == 0
                && get_move_promoted(move_list->moves[count]) == 0)
                // search current move with reduced limit
                score = -negamax(-(alpha+1), -alpha, depth-2);
                
            // hack to ensure that full-depth search is done
            else score = alpha+1;
            
            // if we found a better move during LMR
            if (score > alpha){
                // research at normal depth but with narrowed score bandwith
                score = -negamax( -alpha-1, -alpha, depth-1);

                // if LMR failes, research at full depth and full score bandwidth
                if( score > alpha && (score < beta))
                    score = -negamax(-beta, -alpha, depth -1 );
            }
        }

        
        // decrement ply
        ply--;

        // take move back
        take_back();

        //increment the counter for move search so far
        moves_searched ++;

        // fail hard beta cutoff 1- fail hard frame work, score cant be outside of alpha beta bounds 2- fail soft
        if (score >= beta){

            if (!get_move_capture(move_list->moves[count])){
            //store killer moves
            killer_moves[1][ply] = killer_moves[0][ply];
            killer_moves[0][ply] = move_list->moves[count];
            }
            // nodes fails high
            return beta;
        }

        // found a better move
        if (score > alpha){
            //store history moves
            history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            // PV node (move) 
            alpha = score;
            
            // write PV move
            pv_table[ply][ply] = move_list->moves[count];

            for (int next_ply = ply + 1; next_ply <pv_length[ply + 1]; next_ply++){
                // copy move from deeper ply into a current ply's line
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
            }
            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];




        }
    }

    // there is no legal moves in the current position
    if (legal_moves == 0){
        // king in check
        if (in_check){
            // return mating score
            return -49000 + ply;
        }

        // king is not in check
        else
            // return stalemate score
            return 0; // draw score
    }

    // node (move) fails low
    return alpha;


}

// search position for the best move
void search_position(int depth){
    int score = 0;
    // reset nodes counter
    nodes = 0;

    follow_pv = 0;
    score_pv = 0;

    // clear helper data structure
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_length, 0, sizeof(pv_length));
    memset(pv_table, 0, sizeof(pv_table));
    int alpha, beta;
    alpha = -50000;
    beta = 50000;
    for (int current_depth = 1; current_depth <= depth; current_depth++){
        
        
        // enable follow pv flag
        follow_pv = 1;

        // find best move within a given position
        score = negamax(alpha, beta, current_depth);

        if ((score <= alpha) || (score >= beta)){
            alpha = -50000; // we fell outside the wndow, so try again with a full-width window search 

            beta = 50000;
            continue;
        }

        // set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;
        printf("info score cp %d depth %d nodes %ld pv ", score, current_depth, nodes);
        // loop over the moves a PV line
        for (int count = 0; count < pv_length[0]; count++){
            // print pv move
            print_move(pv_table[0][count]);
            printf(" ");

            }
        printf("\n");

    }

    
    printf("bestmove ");
    print_move(pv_table[0][0]);
    printf("\n");


}



/******************************\
===========================i=====
              UCI
================================
\******************************/

// parse user GUI move string input(e.g., "e7e8q")

int parse_move(char *move_string)
{
    // create move list instance
    moves move_list[1];
    
    // generate moves
    generate_moves(move_list);
    
    // parse source square
    int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
    
    // parse target square
    int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;
    
    // loop over the moves within a move list
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // init move
        int move = move_list->moves[move_count];
        
        // make sure source & target squares are available within the generated move
        if (source_square == get_move_source(move) && target_square == get_move_target(move))
        {
            // init promoted piece
            int promoted_piece = get_move_promoted(move);

            // promoted piece is available
            if (promoted_piece)
            {
                // promoted to queen
                if ((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q')
                    // return legal move
                    return move;
                
                // promoted to rook
                else if ((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r')
                    // return legal move
                    return move;
                
                // promoted to bishop
                else if ((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b')
                    // return legal move
                    return move;
                
                // promoted to knight
                else if ((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n')
                    // return legal move
                    return move;
                
                // continue the loop on possible wrong promotions (e.g. "e7e8f")
                continue;
            }
            
            // return legal move
            return move;
        }
    }
    
    // return illegal move
    return 0;
}

/*
    Example UCI commands to init position on the board

    you can use the starting position using startps command or from a fen string

    // init start position
    position startpos

    // init start position and make the move on chess board
    position startpos moves e2e4 e7e5
    
    // init position from fen string
    position fen r3k2r/p1ppqpb1/bn2pnp1/........

    // init position from fen string and make move on chess board
    position fen r3k2r/p1ppqpb1/bn2pnp1/........ - 0 1 moves e2a6 e8g8

*/

// parse UCI "position" command
void parse_position(char *command)
{
    char *current_char = command + strlen("position");
    while (*current_char == ' ') current_char++;
    
    // parse UCI "startpos" command
    if (strncmp(command, "startpos", 8) == 0)
        // init chess board with start position
        parse_fen(start_position);
    
    // parse UCI "fen" command 
    else
    {
        // make sure "fen" command is available within command string
        current_char = strstr(command, "fen");
        
        // if no "fen" command is available within command string
        if (current_char == NULL)
            // init chess board with start position
            parse_fen(start_position);
            
        // found "fen" substring
        else
        {
            printf("fen found\n");
            // shift pointer to the right where next token begins
            current_char += 4;
            
            // init chess board with position from FEN string
            parse_fen(current_char);
        }
    }
    
    // parse moves after position
    current_char = strstr(command, "moves");
    
    // moves available
    if (current_char != NULL)
    {
        // shift pointer to the right where next token begins
        current_char += 6;
        
        // loop over moves within a move string
        while(*current_char)
        {
            // parse next move
            int move = parse_move(current_char);
            
            // if no more moves
            if (move == 0)
                // break out of the loop
                break;
            
            // make move on the chess board
            make_move(move, all_moves);
            
            // move current character mointer to the end of current move
            while (*current_char && *current_char != ' ') current_char++;
            
            // go to the next move
            current_char++;
        }
        
    }

    print_board();

}

void parse_go(char *command){
    // define depth
    int depth = 6;  // Move default to top
    // init character pointer to the current depth argument
    char *current_depth = strstr(command, "depth");

    // handle fixed depth search
    if (current_depth != NULL)
        // convert string to integer, the result value of depth
        depth = atoi(current_depth + 6);
    // search_postion
    search_position(depth);

    printf("depth: %d\n", depth);



}

/*
    GUI -> isready 
    Engine -> readyok
    GUI -> ucinewgame
*/
// main UCI loop
void uci_loop()
{
    // reset STDIN and STDOUT
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // define user / GUI input buffer
    char input[2000];

    // print engine info
    printf("id name MorphyHead\n");
    printf("id author bohsen\n");
    printf("uciok\n");

    // main loop
    while(1){
        // reset user /GUI input
        memset(input, 0, sizeof(input));

        // make sure output reaches the GUI
        fflush(stdout);

        // get user / GUI input
        if(!fgets(input, 2000, stdin))
            // continue the loop
            continue;

        // make sure the input is available
        if (input[0] == '\n') continue;

        // parse UCI "isready" command
        if (strncmp(input, "isready", 7) == 0){
            printf("readyok\n");
            continue;
        }

        // parse UCI "position" input
        else if (strncmp(input, "position", 8) == 0) parse_position(input);
        
        // parse UCI new game input
        else if (strncmp(input, "ucinewgame", 10) == 0) parse_position("position startpos");
        
        // parse UCI "uci" input
        else if (strncmp(input, "uci", 3) == 0){
            // print engine info
            printf("id name MoriphiesHead\n");
            printf("uciok\n");
        }

        // parse UCI go input
        else if (strncmp(input, "go", 2) == 0) parse_go(input);

        //parse UCI "quit" input
        else if (strncmp(input, "quit", 4) == 0) break;
    }

}


void print_move_score(moves* move_list){
        printf("    Move Scores:\n\n");

        for (int count = 0; count < move_list->count; count++){
            printf("    move: ");
            print_move(move_list->moves[count]);
            printf(" score: %d\n", score_move(move_list->moves[count]));
        }


}

/******************************\
================================
            init all
================================
\******************************/
void init_all(){
    init_leapers_attacks();

    // init slide piecies attacks

    init_sliders_attacks(bishob);
    init_sliders_attacks(rook);
}
int main(){
    // init all attacks bitboards
    init_all();

    // debug variable mode
    int debug = 1;

    // if debug mode
    if(debug){
        parse_fen(tricky_position);
        search_position(11);
    }
    else
        uci_loop();



    return 0;
}