#include "game.h"
#include "parser.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct {
    int id;
    int req_pipe;
    int notif_pipe;
    char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
} Session;



typedef struct {
    const char* levels_dir;
    const int max_games;
    const char* server_pipe_path;
    sem_t session_sem;
    Session* all_sessions;
} Session_manager;

static Session_manager manager;

char* read_pipe_line(int pipe_fd, int* num) {
    char pipe_instruction[MAX_PIPE_PATH_LENGTH*2 + 4];
    char* p = pipe_instruction;
    read_line(pipe_fd, p);
    (*num) = p[0];
    if (*num != 2) {
        p += 2;
    }
    return p;
}

void open_session(Session_manager* manager) {
    for (int i = 0; i < manager->max_games; i++) {
        if (manager->all_sessions[i].id == 0) {
            return i + 1;
        }
    }
}

void* session_thread(void* args) {

}

int main (int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr,
            "Usage: %s <levels_dir> <max_games> <register_file>\n",
            argv[0]);
        return 1;
    }
    const char* levels_dir = argv[1];
    const int max_games = argv[2];
    const char* server_pipe_path = argv[3];
    int current_sessions = 0;
    sem_init(&manager.session_sem, 0, max_games);
    Session* all_sessions = calloc(max_games, sizeof(Session));

    if (mkfifo(server_pipe_path, 0422) != 0) {
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }

    while (1) {
        
        sem_wait(&manager.session_sem);

        if (0) { //END
        
        }

        int num = 0;
        
        int server_pipe_fd = open(server_pipe_path, O_RDONLY);
        char* instruction = read_pipe_line(server_pipe_fd, &num);
        close(server_pipe_fd);

        if (num != 1) {
            perror("Message error in server pipe");
            exit(EXIT_FAILURE);
        }

        if (current_sessions == max_games) {
            //FIX  ME
        }

        Session session;

        sscanf(instruction, "%s %s\n", session.req_pipe_path, session.notif_pipe_path);

        session.req_pipe = open(session.req_pipe_path, O_RDONLY);
        session.notif_pipe = open(session.notif_pipe_path, O_WRONLY);
        
        char buf[3];

        if (session.req_pipe == 0 && session.notif_pipe == 0) {
            sprintf(buf, "%s %s", OP_CODE_CONNECT, 0);
        } else {
            sprintf(buf, "%s %s", OP_CODE_CONNECT, 1);
        }

        write(session.notif_pipe, buf, 3);

    }

    return 0;
}