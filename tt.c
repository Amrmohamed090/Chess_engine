#include "tt.h"

int hash_entires = 0;
tt *hash_table = NULL;

void clear_hash_table(){
    tt *hash_entry;
    for (hash_entry = hash_table; hash_entry < hash_table+hash_entires; hash_entry++){
        hash_entry->hash_key = 0;
        hash_entry->depth = 0;
        hash_entry->flag = 0;
        hash_entry->score = 0;
    }
}

void init_hash_table(int mb){
    int hash_size = 0x100000 * mb;
    hash_entires = hash_size / sizeof(tt);

    if (hash_table != NULL){
        printf("    Clearing hash memory....\n");
        free(hash_table);
    }

    hash_table = (tt *) malloc(hash_entires * sizeof(tt));

    if (hash_table == NULL){
        printf("    Warning: Couldn't allocate memroy for hash table, trying %dMB...", mb/2);
        init_hash_table(mb / 2);
        return;
    }
    clear_hash_table();
    printf("    hash table is initialized with %d entries", hash_entires);
}

int read_hash_entry(int alpha, int beta, int depth){
    tt *hash_entry = &hash_table[hash_key % hash_entires];

    if(hash_entry->hash_key == hash_key){
        if (hash_entry->depth >= depth){
            int score = hash_entry->score;

            if (score < -mate_score) score += ply;
            if (score > mate_score) score -= ply;

            if (hash_entry->flag == hash_flag_exact){return score;}
            if (hash_entry->flag == hash_flag_alpha && score <=alpha){return alpha;}
            if (hash_entry->flag == hash_flag_beta && score >=beta){return beta;}
        }
    }
    return no_hash_entry;
}

void write_hash_entry(int score, int depth, int hash_flag){
    tt *hash_entry = &hash_table[hash_key % hash_entires];

    if (score < -mate_score) score -=ply;
    if (score > mate_score) score += ply;

    hash_entry->hash_key = hash_key;
    hash_entry->score = score;
    hash_entry->flag = hash_flag;
    hash_entry->depth = depth;
}
