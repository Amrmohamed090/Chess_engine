#include "board.h"
#include "hash.h"
#include "bitboard.h"

// piece bitboards
U64 bitboards[12];

// occupancies bitboards
U64 occupancies[3];

// side to move
int side = -1;

// enpassant square
int enpassant = no_sq;

// castling rights
int castle;

// hash key
U64 hash_key;

// positions repetition table
U64 repetitions_table[1000];

// repetition index
int repetition_index;

// half move counter
int ply;

// ASCII pieces
char ascii_pieces[12] = "PNBRQKpnbrqk";

// unicode pieces
char *unicode_pieces[12] = {"♙","♘","♗","♖","♕","♔","♟","♞","♝","♜","♛","♚"};

// convert ascii pieces character to encoded constants
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

// square to coordinates
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

void print_board(){
    for (int rank = 0; rank<8; rank++){
        for ( int file = 0; file < 8; file ++){
            int square = rank * 8 + file;
            if (!file)
            printf(" %d", 8-rank);
            int piece = -1;
            for (int bb_piece = P; bb_piece <= k; bb_piece++){
                if (get_bit(bitboards[bb_piece], square))
                    piece = bb_piece;
            }
            #ifdef WIN64
            printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
            #else
            printf(" %s", ((piece == -1) ? "." : unicode_pieces[piece]));
            #endif
        }
        printf("\n");
    }
    printf("\n    a b c d e f g h\n\n");
    printf("    Side:      %s\n", (!side) ? "w" : "b" );
    printf("    Enpassant: %s\n", (enpassant != no_sq) ? square_to_coordinates[enpassant] : "no");
    printf("    Casteling: %c%c%c%c\n", (castle & wk) ? 'K': '-',
                                            (castle & wq) ? 'Q': '-',
                                            (castle & bk) ? 'k': '-',
                                            (castle & bq) ? 'q': '-');
    printf("    hash key: %llx\n\n", hash_key);
}

void parse_fen(char* fen){
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));
    side = 0;
    enpassant = no_sq;
    castle = 0;
    repetition_index = 0;
    memset(repetitions_table, 0ULL, sizeof(repetitions_table));

    for (int rank = 0; rank<8; rank++){
        for ( int file = 0; file < 8; file++){
            int square = rank * 8 + file;

            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')){
                int piece = char_pieces[(unsigned char)*fen];
                set_bit(bitboards[piece], square);
                fen++;
            }
            if (*fen >= '0' && *fen <= '9'){
                int offset = *fen - '0';
                int piece = -1;
                for (int bb_piece = P; bb_piece <= k; bb_piece++){
                    if (get_bit(bitboards[bb_piece], square))
                        piece = bb_piece;
                }
                if (piece == -1){
                    file --;
                }
                file += offset;
                fen++;
            }
            if (*fen =='/')fen++;
        }
    }
    fen++;

    (*fen == 'w') ? (side = white) : (side = black);
    fen += 2;

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
        int file = fen[0] - 'a';
        int rank = 8-(fen[1] - '0');
        enpassant = rank * 8 + file;
    }
    else enpassant = no_sq;

    for (int piece = P; piece <= K; piece++){
        occupancies[white] |= bitboards[piece];
    }
    for (int piece = p; piece <= k; piece++){
        occupancies[black] |= bitboards[piece];
    }
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];

    hash_key = generate_hash_key();
}
