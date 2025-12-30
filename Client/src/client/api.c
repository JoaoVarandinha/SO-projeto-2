#include "api.h"
#include "protocol.h"
#include "debug.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>


struct Session {
  int id;
  int req_pipe;
  int notif_pipe;
  char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
  char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
};

static struct Session session = {.id = -1};

int pacman_connect(char const *req_pipe_path, char const *notif_pipe_path, char const *server_pipe_path) {

  if(mkfifo(req_pipe_path, 0666) == -1) {
    perror("Error creating request pipe");
    exit(EXIT_FAILURE);
  }

  if(mkfifo(notif_pipe_path, 0666) == -1) {
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
  // TODO - implement me
  return 0;
}

void pacman_play(char command) {

  // TODO - implement me

}

int pacman_disconnect() {
  // TODO - implement me
  return 0;
}

Board receive_board_update(void) {
    // TODO - implement me
}