#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

int main (int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr,
            "Usage: %s <levels_dir> <max_games> <register_file>\n",
            argv[0]);
        return 1;
    }
    const char* levels_dir = argv[1];
    const int max_games = argv[2];
    const char* register_pipe = argv[3];

    if (mkfifo(register_pipe, 0422) != 0) {
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }

    int server_pipe = open(server_pipe_path, O_RDONLY);

    

    return 0;
}