#ifndef UCI_H
#define UCI_H

#include "defs.h"
#include "board.h"
#include "moves.h"

extern int quit;
extern int movestogo;
extern int movetime;
extern int time;
extern int inc;
extern int starttime;
extern int stoptime;
extern int timeset;
extern int stopped;

// function declarations
int get_time_ms(void);
int input_waiting(void);
void read_input(void);
void communicate(void);
int parse_move(char *move_string);
void parse_position(char *command);
void parse_go(char *command);
void uci_loop(void);
void reset_time_control(void);
void print_move_score(moves *move_list);

#endif // UCI_H
