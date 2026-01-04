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


static Session session = {.id = -1};

int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path) {
    unlink(req_pipe_path);
    unlink(notif_pipe_path);
    
    if (mkfifo(req_pipe_path, 0666) == -1) {
        perror("Error creating request pipe");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(notif_pipe_path, 0666) == -1) {
        perror("Error creating notification pipe");
        unlink(req_pipe_path);
        exit(EXIT_FAILURE);
    }

    int server_pipe = open(server_pipe_path, O_WRONLY);

    if (server_pipe == -1) {
        perror("Error opening server pipe");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }

    char op_code = OP_CODE_CONNECT;

    if (write(server_pipe, &op_code, sizeof(char)) != sizeof(char) ||
        write(server_pipe, req_pipe_path, MAX_PIPE_PATH_LENGTH) != MAX_PIPE_PATH_LENGTH ||
        write(server_pipe, notif_pipe_path, MAX_PIPE_PATH_LENGTH) != MAX_PIPE_PATH_LENGTH) {

        perror("Error writing to server pipe - connect");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }

    close(server_pipe);

    if ((session.req_pipe = open(req_pipe_path, O_WRONLY)) == -1) {
        perror("Error opening req/notif pipes");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }
    if ((session.notif_pipe = open(notif_pipe_path, O_RDONLY)) == -1) {
        perror("Error opening req/notif pipes");
        close(session.req_pipe);
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }

    strcpy(session.req_pipe_path, req_pipe_path);
    strcpy(session.notif_pipe_path, notif_pipe_path);

    char buf[sizeof(char)];
    read_char(session.notif_pipe, buf, sizeof(char));

    if (buf[0] != OP_CODE_CONNECT) {
        perror("Error retrieving result for connect");
        exit(EXIT_FAILURE);
    }

    read_char(session.notif_pipe, buf, sizeof(char));

    if (buf[0] == 1) {
        return 1;
    }

    return 0;
}

void pacman_play(char command) {
    char op_code = OP_CODE_PLAY;
    if (write(session.req_pipe, &op_code, sizeof(char)) != sizeof(char) ||
        write(session.req_pipe, &command, sizeof(char)) != sizeof(char)) {

        perror("Error writing to request pipe - play");
        exit(EXIT_FAILURE);
    }
}

int pacman_disconnect() {
    char op_code = OP_CODE_DISCONNECT;
    if (write(session.req_pipe, &op_code, sizeof(char)) != sizeof(char)) {
        perror("Error writing to request pipe - disconnect");
        exit(EXIT_FAILURE);
    }

    close(session.req_pipe);
    close(session.notif_pipe);
    unlink(session.req_pipe_path);
    unlink(session.notif_pipe_path);

    return 0;
}

Board receive_board_update(void) {
    Board board;

    char op_code;
    read_char(session.notif_pipe, &op_code, sizeof(char));

    if (op_code != OP_CODE_BOARD) exit(EXIT_FAILURE);

    read_int(session.notif_pipe, &board.width);
    read_int(session.notif_pipe, &board.height);
    read_int(session.notif_pipe, &board.tempo);
    read_int(session.notif_pipe, &board.victory);
    read_int(session.notif_pipe, &board.game_over);
    read_int(session.notif_pipe, &board.accumulated_points);

    int board_size = board.width*board.height;
    if (!(board.data = calloc(board_size, sizeof(char)))) { 
        perror("Failed to allocate board data");
        exit(EXIT_FAILURE);
    }

    read_char(session.notif_pipe, board.data, board_size);

    return board;
}