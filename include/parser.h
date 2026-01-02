#ifndef PARSER_H
#define PARSER_H

#include "board.h"
#include "game.h"
#define MAX_INSTRUCTION_LENGTH 256

ssize_t read_line(int fd, char* buf);
ssize_t read_char(int fd, char* buf, int bytes);
ssize_t read_int(int fd, int* buf);
char read_request_pipe(Server_session* session);
void read_file(board_t* board, char* filename, char* filetype, int num);
void process_instruction(board_t* board, char* instruction, char* filetype, int* num);
void process_level_instruction(board_t* board, char* instruction, int* num);
void process_pacman_instruction(board_t* board, char* instruction);
void process_ghost_instruction(board_t* board, char* instruction, int num);

#endif
