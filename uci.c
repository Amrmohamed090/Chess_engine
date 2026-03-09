#include "uci.h"
#include "search.h"
#include "perft.h"
#include "tt.h"
#include "eval.h"

// time control variables
int quit = 0;
int movestogo = 30;
int movetime = -1;
int time = -1;
int inc = 0;
int starttime = 0;
int stoptime = 0;
int timeset = 0;
int stopped = 0;

int get_time_ms()
{
    #ifdef WIN64
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif
}

int input_waiting()
{
    #ifndef WIN32
        fd_set readfds;
        struct timeval tv;
        FD_ZERO (&readfds);
        FD_SET (fileno(stdin), &readfds);
        tv.tv_sec=0; tv.tv_usec=0;
        select(16, &readfds, 0, 0, &tv);
        return (FD_ISSET(fileno(stdin), &readfds));
    #else
        static int init = 0, pipe;
        static HANDLE inh;
        DWORD dw;

        if (!init)
        {
            init = 1;
            inh = GetStdHandle(STD_INPUT_HANDLE);
            pipe = !GetConsoleMode(inh, &dw);
            if (!pipe)
            {
                SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
                FlushConsoleInputBuffer(inh);
            }
        }

        if (pipe)
        {
           if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
           return dw;
        }
        else
        {
           GetNumberOfConsoleInputEvents(inh, &dw);
           return dw <= 1 ? 0 : dw;
        }
    #endif
}

void read_input()
{
    int bytes;
    char input[256] = "", *endc;

    if (input_waiting())
    {
        stopped = 1;

        do
        {
            bytes=read(fileno(stdin), input, 256);
        }
        while (bytes < 0);

        endc = strchr(input,'\n');
        if (endc) *endc=0;

        if (strlen(input) > 0)
        {
            if (!strncmp(input, "quit", 4))
            {
                quit = 1;
            }
            else if (!strncmp(input, "stop", 4)){
                quit = 1;
            }
        }
    }
}

void communicate() {
    if(timeset == 1 && get_time_ms() > stoptime) {
        stopped = 1;
    }
    read_input();
}

int parse_move(char *move_string)
{
    moves move_list[1];
    generate_moves(move_list);

    int source_square = (move_string[0] - 'a') + (8 - (move_string[1] - '0')) * 8;
    int target_square = (move_string[2] - 'a') + (8 - (move_string[3] - '0')) * 8;

    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        int move = move_list->moves[move_count];

        if (source_square == get_move_source(move) && target_square == get_move_target(move))
        {
            int promoted_piece = get_move_promoted(move);

            if (promoted_piece)
            {
                if ((promoted_piece == Q || promoted_piece == q) && move_string[4] == 'q')
                    return move;
                else if ((promoted_piece == R || promoted_piece == r) && move_string[4] == 'r')
                    return move;
                else if ((promoted_piece == B || promoted_piece == b) && move_string[4] == 'b')
                    return move;
                else if ((promoted_piece == N || promoted_piece == n) && move_string[4] == 'n')
                    return move;
                continue;
            }

            return move;
        }
    }

    return 0;
}

void parse_position(char *command)
{
    command += 9;
    char *current_char = command;

    if (strncmp(command, "startpos", 8) == 0)
        parse_fen(start_position);
    else
    {
        current_char = strstr(command, "fen");

        if (current_char == NULL)
            parse_fen(start_position);
        else
        {
            current_char += 4;
            parse_fen(current_char);
        }
    }

    current_char = strstr(command, "moves");

    if (current_char != NULL)
    {
        current_char += 6;

        while(*current_char)
        {
            int move = parse_move(current_char);

            if (move == 0)
                break;

            repetition_index ++;
            repetitions_table[repetition_index] = hash_key;

            make_move(move, all_moves);

            while (*current_char && *current_char != ' ') current_char++;
            current_char++;
        }
    }

    print_board();
}

void reset_time_control()
{
    quit = 0;
    movestogo = 30;
    movetime = -1;
    time = -1;
    inc = 0;
    starttime = 0;
    stoptime = 0;
    timeset = 0;
    stopped = 0;
}

void parse_go(char *command)
{
    reset_time_control();

    int depth = -1;
    char *argument = NULL;

    if ((argument = strstr(command,"infinite"))) {}

    if ((argument = strstr(command,"binc")) && side == black)
        inc = atoi(argument + 5);

    if ((argument = strstr(command,"winc")) && side == white)
        inc = atoi(argument + 5);

    if ((argument = strstr(command,"wtime")) && side == white)
        time = atoi(argument + 6);

    if ((argument = strstr(command,"btime")) && side == black)
        time = atoi(argument + 6);

    if ((argument = strstr(command,"movestogo")))
        movestogo = atoi(argument + 10);

    if ((argument = strstr(command,"movetime")))
        movetime = atoi(argument + 9);

    if ((argument = strstr(command,"depth")))
        depth = atoi(argument + 6);

    if(movetime != -1)
    {
        time = movetime;
        movestogo = 1;
    }

    starttime = get_time_ms();

    if(time != -1)
    {
        timeset = 1;
        time /= movestogo;

        if (time > 1500)
            time -= 50;

        stoptime = starttime + time + inc;
    }

    if(depth == -1)
        depth = 64;

    printf("time:%d start:%u stop:%u depth:%d timeset:%d\n",
            time, starttime, stoptime, depth, timeset);

    search_position(depth);
}

void uci_loop()
{
    int max_hash = 128;
    int mb = 64;

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    char input[2000];

    printf("id name BBC\n");
    printf("id author Code Monkey King\n");
    printf("option name Hash type spin default 64 min 4 max %d\n", max_hash);
    printf("uciok\n");

    while (1)
    {
        memset(input, 0, sizeof(input));
        fflush(stdout);

        if (!fgets(input, 2000, stdin))
            continue;

        if (input[0] == '\n')
            continue;

        if (strncmp(input, "isready", 7) == 0)
        {
            printf("readyok\n");
            continue;
        }
        else if (strncmp(input, "position", 8) == 0)
        {
            parse_position(input);
            clear_hash_table();
        }
        else if (strncmp(input, "ucinewgame", 10) == 0)
        {
            parse_position("position startpos");
            clear_hash_table();
        }
        else if (strncmp(input, "go", 2) == 0)
            parse_go(input);
        else if (strncmp(input, "quit", 4) == 0)
            break;
        else if (strncmp(input, "eval", 4) == 0)
        {
            printf("eval %d\n", evaluate());
            fflush(stdout);
        }
        else if (strncmp(input, "uci", 3) == 0)
        {
            printf("id name BBC\n");
            printf("id author Code Monkey King\n");
            printf("uciok\n");
        }
        else if (!strncmp(input, "setoption name Hash value ", 26)) {
            sscanf(input,"%*s %*s %*s %*s %d", &mb);

            if(mb < 4) mb = 4;
            if(mb >= max_hash) mb = max_hash;

            printf("    Set hash table size to %dMB\n", mb);
            init_hash_table(mb);
        }
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
