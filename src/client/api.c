#include "api.h"
#include "protocol.h"
#include "debug.h"
#include "parser.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>


typedef struct {
    int id;
    int req_pipe;
    int notif_pipe;
    char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
} Session;

typedef struct {
    char op_code;
    char req_pipe_path[MAX_PIPE_PATH_LENGTH];
    char notif_pipe_path[MAX_PIPE_PATH_LENGTH];
}Pac_Connect_req_pipe;


typedef struct {
    char op_code;
    char command;
}Pac_Play_req_pipe;

static Session session = {.id = -1};
static Board board;

int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path) {

    if(mkfifo(req_pipe_path, 0244) == -1) {
        perror("Error creating request pipe");
        exit(EXIT_FAILURE);
    }

    if(mkfifo(notif_pipe_path, 0422) == -1) {
        perror("Error creating notification pipe");
        unlink(req_pipe_path);
        exit(EXIT_FAILURE);
    }

    int server_pipe = open(server_pipe_path, O_WRONLY);

    if(server_pipe == -1) {
        perror("Error opening server pipe");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }

    Pac_Connect_req_pipe connect_req_pipe;

    connect_req_pipe.op_code = OP_CODE_CONNECT;
    strcpy(connect_req_pipe.req_pipe_path, req_pipe_path);
    strcpy(connect_req_pipe.notif_pipe_path, notif_pipe_path);

    if (write(server_pipe_path, connect_req_pipe, sizeof(connect_req_pipe)) != sizeof(connect_req_pipe)) {
        perror("Error writing to server pipe - connect");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }

    
    close(server_pipe);
  
    strcpy(session.req_pipe_path, req_pipe_path);
    strcpy(session.notif_pipe_path, notif_pipe_path);
    session.req_pipe = open(session.req_pipe_path, O_WRONLY);
    session.notif_pipe = open(session.notif_pipe_path, O_RDONLY);


    //FIX ME
    read(session.notif_pipe_path, buf, 3);

    if (buf[0] != OP_CODE_CONNECT) {
        perror("Error retrieving result for connect");
        exit(EXIT_FAILURE);
    }

    return buf[2];
}

void pacman_play(char command) {
    Pac_Play_req_pipe play_req_pipe;

    play_req_pipe.op_code = OP_CODE_PLAY;
    play_req_pipe.command = command;
    
    if(write(session.req_pipe, &play_req_pipe, sizeof(play_req_pipe)) != sizeof(play_req_pipe)) {
        perror("Error writing to request pipe - play");
        exit(EXIT_FAILURE)
    }
}

int pacman_disconnect() {
    if (write(session.req_pipe, OP_CODE_DISCONNECT, sizeof(char)) != sizeof(char)) {
        perror("Error writing to request pipe");
        exit(EXIT_FAILURE);
    }
  
    unlink(session.req_pipe_path);
    unlink(session.notif_pipe_path);

    return 0;
}

Board receive_board_update(void) {
    Board board;

    char op_code;
    if (read(session.notif_pipe, &op_code, 1) != 1) exit(EXIT_FAILURE);

    if (op_code != OP_CODE_BOARD) exit(EXIT_FAILURE);

    if (read(session.notif_pipe, &board.width, sizeof(int)) != sizeof(int)) {
        perror("Error reading board width");
        exit(EXIT_FAILURE);
    }

    if (read(session.notif_pipe, &board.height, sizeof(int)) != sizeof(int)) {
        perror("Error reading board height");
        exit(EXIT_FAILURE);
    } 
    
    if (read(session.notif_pipe, &board.tempo, sizeof(int)) != sizeof(int)) {
        perror("Error reading board tempo");
        exit(EXIT_FAILURE);
    } 

    if (read(session.notif_pipe, &board.victory, sizeof(int)) != sizeof(int)) {
        perror("Error reading board victory");
        exit(EXIT_FAILURE);
    }

    if (read(session.notif_pipe, &board.game_over, sizeof(int)) != sizeof(int)) {
        perror("Error reading board game_over");
        exit(EXIT_FAILURE);
    }

    if (read(session.notif_pipe, &board.accumulated_points, sizeof(int)) != sizeof(int)) {
        perror("Error reading board accumulated points");
        exit(EXIT_FAILURE);
    }
    
    int board_size = board.width*board.height;
    board.data = calloc(1, sizeof(board_size));
    if(read(session.notif_pipe, board.data, board_size) != board_size) {
        free(board.data);
        perror("Error reading board data");
        exit(EXIT_FAILURE);
    } 
    
    return board;
}