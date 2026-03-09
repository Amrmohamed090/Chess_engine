#include "perft.h"
#include "hash.h"
#include "board.h"
#include "uci.h"

// nodes is defined in search.c, declared extern in search.h and perft.h

void perft_driver(int depth){
    if (depth == 0){
        nodes++;
        return;
    }

    moves move_list[1];
    generate_moves(move_list);

    for (int move_count = 0; move_count < move_list->count; move_count++){
        int move = move_list->moves[move_count];
        copy_board();
        if (!make_move(move,all_moves))
            continue;

        take_back();

        U64 hash_from_scratch = generate_hash_key();

        if(hash_key != hash_from_scratch){
            printf("\nTake Back\n");
            printf("move: "); print_move(move_list->moves[move_count]);
            printf("\n");
            print_board();
            printf("hash key should be: %llx\n", hash_from_scratch);
            break;
        }
        perft_driver(depth - 1);
    }
}

void perft_test(int depth){
    printf("    Performance Test \n\n");
    moves move_list[1];
    generate_moves(move_list);

    long start_time = get_time_ms();
    for (int move_count = 0; move_count < move_list->count; move_count++){
        int move = move_list->moves[move_count];
        copy_board();
        if (!make_move(move,all_moves))
            continue;

        long cumulative_nodes = nodes;
        perft_driver(depth - 1);
        long old_nodes = nodes - cumulative_nodes;
        printf("    move: %s%s%c  nodes: %ld \n",
                        square_to_coordinates[get_move_source(move)],
                        square_to_coordinates[get_move_target(move)],
                        promoted_pieces[get_move_promoted(move)]
                        , old_nodes);
        take_back();
    }

    printf("\n    Depth: %d\n", depth);
    printf("    Nodes: %lld\n", nodes);
    printf("    Time:  %ldms\n", get_time_ms()-start_time);
}
