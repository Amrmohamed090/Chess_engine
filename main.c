#include "defs.h"
#include "attacks.h"
#include "hash.h"
#include "eval.h"
#include "tt.h"
#include "uci.h"
#include "board.h"

void init_all(){
    init_leapers_attacks();
    init_sliders_attacks(bishob);
    init_sliders_attacks(rook);
    init_hash_keys();
    initialize_evaluation_masks();
    init_hash_table(12);
}

int main(){
    init_all();
    int debug = 0;

    if (debug){
        parse_fen("6k1/ppppprbp/8/8/8/8/PPPPPRBP/6K1 w - -");
        print_board();
        printf("score: %d\n", evaluate());
    }
    else
        uci_loop();

    return 0;
}
