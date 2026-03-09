#include "moves.h"

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
        printf("%s%s%c    %c        %d         %d         %d           %d  \n",
                square_to_coordinates[get_move_source(move)],
                square_to_coordinates[get_move_target(move)],
                (get_move_promoted(move)? promoted_pieces[get_move_promoted(move)] : ' '),
                ascii_pieces[get_move_piece(move)],
                (get_move_capture(move)? 1: 0),
                (get_move_double(move)? 1: 0),
                (get_move_enpassant(move)? 1: 0),
                (get_move_castle(move) )? 1: 0);
    }
}

int make_move(int move, int move_flag)
{
    if (move_flag == all_moves)
    {
        copy_board();

        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castle(move);

        pop_bit(bitboards[piece], source_square);
        set_bit(bitboards[piece], target_square);

        hash_key ^= piece_keys[piece][source_square];
        hash_key ^= piece_keys[piece][target_square];

        if (capture)
        {
            int start_piece, end_piece;

            if (side == white)
            {
                start_piece = p;
                end_piece = k;
            }
            else
            {
                start_piece = P;
                end_piece = K;
            }

            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
            {
                if (get_bit(bitboards[bb_piece], target_square))
                {
                    pop_bit(bitboards[bb_piece], target_square);
                    hash_key ^= piece_keys[bb_piece][target_square];
                    break;
                }
            }
        }

        if (promoted_piece)
        {
            if (side == white)
            {
                pop_bit(bitboards[P], target_square);
                hash_key ^= piece_keys[P][target_square];
            }
            else
            {
                pop_bit(bitboards[p], target_square);
                hash_key ^= piece_keys[p][target_square];
            }

            set_bit(bitboards[promoted_piece], target_square);
            hash_key ^= piece_keys[promoted_piece][target_square];
        }

        if (enpass)
        {
            (side == white) ? pop_bit(bitboards[p], target_square + 8) :
                              pop_bit(bitboards[P], target_square - 8);

            if (side == white)
            {
                pop_bit(bitboards[p], target_square + 8);
                hash_key ^= piece_keys[p][target_square + 8];
            }
            else
            {
                pop_bit(bitboards[P], target_square - 8);
                hash_key ^= piece_keys[P][target_square - 8];
            }
        }

        if (enpassant != no_sq) hash_key ^= enpassant_keys[enpassant];
        enpassant = no_sq;

        if (double_push)
        {
            if (side == white)
            {
                enpassant = target_square + 8;
                hash_key ^= enpassant_keys[target_square + 8];
            }
            else
            {
                enpassant = target_square - 8;
                hash_key ^= enpassant_keys[target_square - 8];
            }
        }

        if (castling)
        {
            switch (target_square)
            {
                case (g1):
                    pop_bit(bitboards[R], h1);
                    set_bit(bitboards[R], f1);
                    hash_key ^= piece_keys[R][h1];
                    hash_key ^= piece_keys[R][f1];
                    break;

                case (c1):
                    pop_bit(bitboards[R], a1);
                    set_bit(bitboards[R], d1);
                    hash_key ^= piece_keys[R][a1];
                    hash_key ^= piece_keys[R][d1];
                    break;

                case (g8):
                    pop_bit(bitboards[r], h8);
                    set_bit(bitboards[r], f8);
                    hash_key ^= piece_keys[r][h8];
                    hash_key ^= piece_keys[r][f8];
                    break;

                case (c8):
                    pop_bit(bitboards[r], a8);
                    set_bit(bitboards[r], d8);
                    hash_key ^= piece_keys[r][a8];
                    hash_key ^= piece_keys[r][d8];
                    break;
            }
        }

        hash_key ^= castle_keys[castle];
        castle &= castling_rights[source_square];
        castle &= castling_rights[target_square];
        hash_key ^= castle_keys[castle];

        memset(occupancies, 0ULL, 24);

        for (int bb_piece = P; bb_piece <= K; bb_piece++)
            occupancies[white] |= bitboards[bb_piece];
        for (int bb_piece = p; bb_piece <= k; bb_piece++)
            occupancies[black] |= bitboards[bb_piece];
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        side ^= 1;
        hash_key ^= side_key;

        if (is_square_attacked((side == white) ? get_ls1b_index(bitboards[k]) : get_ls1b_index(bitboards[K]), side))
        {
            take_back();
            return 0;
        }
        else
            return 1;
    }
    else
    {
        if (get_move_capture(move))
            return make_move(move, all_moves);
        else
            return 0;
    }
}

void generate_moves(moves *move_list){
    move_list->count = 0;
    int source_square, target_square;
    U64 bitboard, attacks;

    for (int piece = P; piece <= k; piece++){
        bitboard = bitboards[piece];

        if (side == white){
            if (piece == P){
                while (bitboard)
                {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square - 8;

                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square)){
                        if (source_square >= a7 && source_square <= h7){
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
                    attacks = pawn_attacks[side][source_square] & occupancies[black];

                    while (attacks){
                        target_square = get_ls1b_index(attacks);
                        if (source_square >= a7 && source_square <= h7){
                            add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }
                        else{
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }

                    if (enpassant != no_sq){
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks){
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == K){
                if (castle & wk){
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1)){
                        if (!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                            add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }
                if (castle & wq){
                    if (!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], b1)){
                        if (!is_square_attacked(e1, black) && !is_square_attacked(d1, black))
                            add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }
        else{
            if (piece == p){
                while (bitboard)
                {
                    source_square = get_ls1b_index(bitboard);
                    target_square = source_square + 8;

                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square)){
                        if (source_square >= a2 && source_square <= h2){
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }
                        else{
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            if((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square+8))
                                add_move(move_list, encode_move(source_square, target_square+8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    attacks = pawn_attacks[side][source_square] & occupancies[white];

                    while (attacks){
                        target_square = get_ls1b_index(attacks);
                        if (source_square >= a2 && source_square <= h2){
                            add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }
                        else{
                            add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        pop_bit(attacks, target_square);
                    }

                    if (enpassant != no_sq){
                        U64 enpassant_attacks = pawn_attacks[side][source_square] & (1ULL << enpassant);
                        if (enpassant_attacks){
                            int target_enpassant = get_ls1b_index(enpassant_attacks);
                            add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == k){
                if (castle & bk){
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8)){
                        if (!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                            add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }
                if (castle & bq){
                    if (!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], b8)){
                        if (!is_square_attacked(e8, white) && !is_square_attacked(d8, white) && !is_square_attacked(c8, white))
                            add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        if ((side == white) ? piece == N: piece == n){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }

        if ((side == white) ? piece == B: piece == b){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = get_bishob_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }

        if ((side == white) ? piece == R: piece == r){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }

        if ((side == white) ? piece == Q: piece == q){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }

        if ((side == white) ? piece == K: piece == k){
            while (bitboard){
                source_square = get_ls1b_index(bitboard);
                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);
                while (attacks){
                    target_square = get_ls1b_index(attacks);
                    if (!get_bit(((side == white) ? occupancies[black] : occupancies[white]), target_square))
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    pop_bit(attacks, target_square);
                }
                pop_bit(bitboard, source_square);
            }
        }
    }
}
