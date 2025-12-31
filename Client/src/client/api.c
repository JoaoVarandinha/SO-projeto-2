#include "api.h"
#include "protocol.h"
#include "debug.h"

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

/*
struct Connect_request {
  char op_code;
  char req_pipe_name[MAX_PIPE_NAME];
  char notif_pipe_name[MAX_PIPE_NAME];   
};

struct Connect_response {
  char op_code;
  char result;
};
*/

static Session session = {.id = -1};

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

    char buf[MAX_PIPE_PATH_LENGTH*2 + 4];
    sprintf(buf, "%s %s %s\n", OP_CODE_CONNECT, req_pipe_path, notif_pipe_path);
    
    ssize_t bytes = write(server_pipe_path, buf, sizeof(buf));
    close(server_pipe);
    if(bytes != sizeof(buf)) {
        perror("Error writing to server pipe");
        unlink(req_pipe_path);
        unlink(notif_pipe_path);
        exit(EXIT_FAILURE);
    }
  
    strcpy(session.req_pipe_path, req_pipe_path);
    strcpy(session.notif_pipe_path, notif_pipe_path);
    session.req_pipe = open(session.req_pipe_path, O_WRONLY);
    session.notif_pipe = open(session.notif_pipe_path, O_RDONLY);

    read(session.notif_pipe_path, buf, 3);

    if (buf[0] != OP_CODE_CONNECT) {
        perror("Error retrieving result for connect");
        exit(EXIT_FAILURE);
    }

    return buf[2];
}

void pacman_play(char command) {
    char buf[3];
    sprintf(buf, "%s %s\n", OP_CODE_PLAY, command);

    ssize_t bytes = write(session.req_pipe, buf, sizeof(buf));
    if(bytes != sizeof(buf)) {
        perror("Error writing to request pipe");
        exit(EXIT_FAILURE);
    }


    // TODO - implement me

}

int pacman_disconnect() {
    char buf[1];
    sprintf(buf, "%s\n", OP_CODE_DISCONNECT);

    ssize_t bytes = write(session.req_pipe, buf, sizeof(buf));
    if(bytes != sizeof(buf)) {
        perror("Error writing to request pipe");
        exit(EXIT_FAILURE);
    }
    
    unlink(session.req_pipe_path);
    unlink(session.notif_pipe_path);

    return 0;
}

Board receive_board_update(void) {
    // TODO - implement me
}