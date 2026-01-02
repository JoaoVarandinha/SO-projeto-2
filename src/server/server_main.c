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

static Server_manager manager;

Server_session* find_open_session() {
    for (int i = 0; i < manager.max_games; i++) {
        if (manager.all_sessions[i].running == 0) {
            return &manager.all_sessions[i];
        }
    }
    perror("Error finding an open session");
    exit(EXIT_FAILURE);
}

int check_ongoing_sessions() {
    pthread_mutex_lock(&manager.server_lock);
    int i = manager.ongoing_sessions;
    pthread_mutex_unlock(&manager.server_lock);
    return i;
}

void change_ongoing_sessions(int i) {
    pthread_mutex_lock(&manager.server_lock);
    manager.ongoing_sessions += i;
    pthread_mutex_unlock(&manager.server_lock);
}

void* session_thread(void* arg) {
    Server_session* session = (Server_session*) arg;
    sem_init(&session->session_sem, 0, 0);
    pthread_mutex_init(&session->session_lock, NULL);

    while (1) {
        if (0) { //END
            break;
        }
        sem_wait(&session->session_sem);
        change_ongoing_sessions(1);
        pthread_mutex_lock(&session->session_lock);
        session->running = 1;
        pthread_mutex_unlock(&session->session_lock);

        while (1) {
            char instruction[MAX_INSTRUCTION_LENGTH];
            read_line(session->req_pipe, instruction);

            if (instruction[0] == OP_CODE_DISCONNECT) break;

            else if(instruction[0] == OP_CODE_PLAY) {

            } else if(instruction[0] == OP_CODE_BOARD) {

            }
        }

        pthread_mutex_lock(&session->session_lock);
        session->running = 0;
        pthread_mutex_unlock(&session->session_lock);
        change_ongoing_sessions(-1);
        sem_post(&manager.server_sem);
    }

    sem_destroy(&session->session_sem);
    pthread_mutex_destroy(&session->session_lock);

    return NULL;
}

int main (int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr,
            "Usage: %s <levels_dir> <max_games> <register_file>\n",
            argv[0]);
        return 1;
    }

    manager.levels_dir = argv[1];
    manager.max_games = argv[2];
    manager.server_pipe_path = argv[3];
    manager.ongoing_sessions = 0;
    sem_init(&manager.server_sem, 0, manager.max_games);
    pthread_mutex_init(&manager.server_lock, NULL);
    manager.all_sessions = calloc(manager.max_games, sizeof(Server_session));

    if (mkfifo(manager.server_pipe_path, 0422) != 0) {
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }

    pthread_t session_tid[manager.max_games];

    for (int i = 0; i < manager.max_games; i++) {
        manager.all_sessions[i].id = i + 1;
        pthread_create(&session_tid[i], NULL, session_thread, &manager.all_sessions[i]);
    }


    while (1) {
        
        sem_wait(&manager.server_sem);

        if (0) { //END
            break;
        }

        if (check_ongoing_sessions() == manager.max_games) {
            perror("Error creating session with no available slots");
            exit(EXIT_FAILURE);
        }

        int server_pipe_fd = open(manager.server_pipe_path, O_RDONLY);
        char* instruction[MAX_PIPE_PATH_LENGTH*2 + 4];
        read_line(server_pipe_fd, instruction);
        close(server_pipe_fd);

        if (instruction[0] != 1) {
            perror("Message error in server pipe");
            exit(EXIT_FAILURE);
        }

        Server_session* session = find_open_session();

        sscanf(instruction, "1 %s %s\n", session->req_pipe_path, session->notif_pipe_path);

        session->req_pipe = open(session->req_pipe_path, O_RDONLY);
        session->notif_pipe = open(session->notif_pipe_path, O_WRONLY);

        char buf[3];

        if (session->req_pipe == 0 && session->notif_pipe == 0) {
            sprintf(buf, "%s %s\n", OP_CODE_CONNECT, 0);
            sem_post(&session->session_sem);
        } else {
            sprintf(buf, "%s %s\n", OP_CODE_CONNECT, 1);
        }

        write(session->notif_pipe, buf, 3);

    }


    for (int i = 0; i < manager.max_games; i++) {
        pthread_join(&session_tid[i], NULL);
    }
    sem_destroy(&manager.server_sem);
    pthread_mutex_destroy(&manager.server_lock);
    free(manager.all_sessions);

    return 0;
}