#include "search.h"
#include "uci.h"

// search state variables
int killer_moves[2][max_ply];
int history_moves[12][64];
int pv_length[max_ply];
int pv_table[max_ply][max_ply];
int follow_pv;
int score_pv;
U64 nodes;

// LMR constants
static const int full_depth_moves = 4;
static const int reduction_limit = 3;

// MVV LVA [attacker][victim]
static int mvv_lva[12][12] = {
    {105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605},
    {104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604},
    {103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603},
    {102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602},
    {101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601},
    {100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600},
    {105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605},
    {104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604},
    {103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603},
    {102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602},
    {101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601},
    {100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600},
};

void enable_pv_scoring(moves* move_list){
    follow_pv = 0;
    for (int count = 0; count < move_list->count; count++){
        if (pv_table[0][ply] == move_list->moves[count]){
            score_pv = 1;
            follow_pv = 1;
        }
    }
}

int score_move(int move){
    if (score_pv){
        if (pv_table[0][ply] == move){
            score_pv = 0;
            return 20000;
        }
    }

    if (get_move_capture(move)){
        int target_piece = p;
        int start_piece, end_piece;

        if (side == white){ start_piece = p; end_piece = k;}
        else{ start_piece = P; end_piece = K;}

        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++){
            if (get_bit(bitboards[bb_piece], get_move_target(move))){
                target_piece = bb_piece;
                break;
            }
        }

        return mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }
    else{
        if (killer_moves[0][ply] == move)
            return 9000;
        else if (killer_moves[1][ply] == move)
            return 8000;
        else
            return history_moves[get_move_piece(move)][get_move_target(move)];
    }
    return 0;
}

void sort_moves(moves *move_list)
{
    int move_scores[move_list->count];

    for (int count = 0; count < move_list->count; count++)
        move_scores[count] = score_move(move_list->moves[count]);

    for (int current_move = 0; current_move < move_list->count; current_move++)
    {
        for (int next_move = current_move + 1; next_move < move_list->count; next_move++)
        {
            if (move_scores[current_move] < move_scores[next_move])
            {
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                int temp_move = move_list->moves[current_move];
                move_list->moves[current_move] = move_list->moves[next_move];
                move_list->moves[next_move] = temp_move;
            }
        }
    }
}

int is_repetition(){
    for (int index = 0; index < repetition_index; index++)
        if (repetitions_table[index] == hash_key)
            return 1;
    return 0;
}

int quiescense(int alpha, int beta){
    if((nodes & 2047 ) == 0)
        communicate();

    nodes++;

    if (ply > max_ply - 1)
        return evaluate();

    int evaluation = evaluate();

    if (evaluation >= beta){
        return beta;
    }

    if (evaluation > alpha){
        alpha = evaluation;
    }

    moves move_list[1];
    generate_moves(move_list);
    sort_moves(move_list);

    for (int count = 0; count<move_list->count; count++){
        copy_board();

        ply++;
        repetition_index++;
        repetitions_table[repetition_index] = hash_key;

        if (make_move(move_list->moves[count], only_captures) == 0){
            ply--;
            repetition_index--;
            continue;
        }

        int score = -quiescense(-beta, -alpha);

        ply--;
        repetition_index--;
        take_back();

        if (score >= beta){
            return beta;
        }

        if (score > alpha){
            alpha = score;
        }
    }

    return alpha;
}

int negamax(int alpha, int beta, int depth)
{
    int score;
    int hash_flag = hash_flag_alpha;

    if (ply && is_repetition())
        return 0;

    int pv_node = beta - alpha > 1;

    if (ply && (score = read_hash_entry(alpha, beta, depth)) != no_hash_entry && pv_node == 0)
        return score;

    if((nodes & 2047 ) == 0)
        communicate();

    pv_length[ply] = ply;

    if (depth == 0)
        return quiescense(alpha, beta);

    if (ply > max_ply - 1)
        return evaluate();

    nodes++;

    int in_check = is_square_attacked((side == white) ? get_ls1b_index(bitboards[K]) :
                                                        get_ls1b_index(bitboards[k]),
                                                        side ^ 1);

    if (in_check) depth++;

    int legal_moves = 0;

    if (depth >= 3 && in_check == 0 && ply)
    {
        copy_board();

        ply++;
        repetition_index++;
        repetitions_table[repetition_index] = hash_key;

        if (enpassant != no_sq) hash_key ^= enpassant_keys[enpassant];
        enpassant = no_sq;

        side ^= 1;
        hash_key ^= side_key;

        score = -negamax(-beta, -beta + 1, depth - 1 - 2);

        ply--;
        repetition_index--;
        take_back();

        if(stopped == 1) return 0;

        if (score >= beta)
            return beta;
    }

    moves move_list[1];
    generate_moves(move_list);

    if (follow_pv)
        enable_pv_scoring(move_list);

    sort_moves(move_list);

    int moves_searched = 0;

    for (int count = 0; count < move_list->count; count++)
    {
        copy_board();

        ply++;
        repetition_index++;
        repetitions_table[repetition_index] = hash_key;

        if (make_move(move_list->moves[count], all_moves) == 0)
        {
            ply--;
            repetition_index--;
            continue;
        }

        legal_moves++;

        if (moves_searched == 0)
            score = -negamax(-beta, -alpha, depth - 1);
        else
        {
            if(
                moves_searched >= full_depth_moves &&
                depth >= reduction_limit &&
                in_check == 0 &&
                get_move_capture(move_list->moves[count]) == 0 &&
                get_move_promoted(move_list->moves[count]) == 0
              )
                score = -negamax(-alpha - 1, -alpha, depth - 2);
            else score = alpha + 1;

            if(score > alpha)
            {
                score = -negamax(-alpha - 1, -alpha, depth-1);

                if((score > alpha) && (score < beta))
                    score = -negamax(-beta, -alpha, depth-1);
            }
        }

        ply--;
        repetition_index--;
        take_back();

        if(stopped == 1) return 0;

        moves_searched++;

        if (score > alpha)
        {
            hash_flag = hash_flag_exact;

            if (get_move_capture(move_list->moves[count]) == 0)
                history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            alpha = score;

            pv_table[ply][ply] = move_list->moves[count];

            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];

            pv_length[ply] = pv_length[ply + 1];

            if (score >= beta)
            {
                write_hash_entry(beta, depth, hash_flag_beta);

                if (get_move_capture(move_list->moves[count]) == 0)
                {
                    killer_moves[1][ply] = killer_moves[0][ply];
                    killer_moves[0][ply] = move_list->moves[count];
                }

                return beta;
            }
        }
    }

    if (legal_moves == 0)
    {
        if (in_check)
            return -mate_value + ply;
        else
            return 0;
    }

    write_hash_entry(alpha, depth, hash_flag);

    return alpha;
}

void search_position(int depth){
    int score = 0;
    int best_move = 0;
    nodes = 0;
    stopped = 0;
    follow_pv = 0;
    score_pv = 0;

    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    memset(pv_length, 0, sizeof(pv_length));
    memset(pv_table, 0, sizeof(pv_table));

    clear_hash_table();

    int alpha, beta;
    alpha = -infinity;
    beta = infinity;

    for (int current_depth = 1; current_depth <= depth; current_depth++){
        follow_pv = 1;
        score = negamax(alpha, beta, current_depth);

        if (stopped == 1)
            break;

        if ((score <= alpha) || (score >= beta)){
            alpha = -infinity;
            beta = infinity;
            current_depth--;
            continue;
        }

        alpha = score - 50;
        beta = score + 50;

        best_move = pv_table[0][0];
        if (pv_length[0]){
            if (score > -mate_value && score < -mate_score)
                printf("info score mate %d depth %d nodes %lld time %d pv ", -(score + mate_value) / 2 - 1, current_depth, nodes, get_time_ms() - starttime);
            else if (score > mate_value && score < mate_score)
                printf("info score mate %d depth %d nodes %lld time %d pv ", (mate_value - score) / 2 + 1, current_depth, nodes, get_time_ms() - starttime);
            else
                printf("info score cp %d depth %d nodes %lld time %d pv ", score, current_depth, nodes, get_time_ms() - starttime);

            for (int count = 0; count < pv_length[0]; count++){
                print_move(pv_table[0][count]);
                printf(" ");
            }
            printf("\n");
        }
    }
    printf("bestmove ");
    print_move(best_move);
    printf("\n");
}
