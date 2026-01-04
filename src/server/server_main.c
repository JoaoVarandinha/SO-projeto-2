#include "game.h"
#include "parser.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

static Server_manager manager;

Server_session* find_open_session() {
    for (int i = 0; i < manager.max_games; i++) {
        Server_session* session = &manager.all_sessions[i];
        pthread_mutex_lock(&session->session_lock);
        if (session->running == 0) {
            session->running = 1;
            pthread_mutex_unlock(&session->session_lock);
            return session;
        }
        pthread_mutex_unlock(&session->session_lock);
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

void end_session(Server_session* session) {
    pthread_mutex_lock(&session->session_lock);
    session->running = 0;
    pthread_mutex_unlock(&session->session_lock);
    change_ongoing_sessions(-1);
    sem_post(&manager.server_sem);
}

void* session_thread(void* arg) {
    Server_session* session = (Server_session*) arg;
    sem_init(&session->session_sem, 0, 0);
    pthread_mutex_init(&session->session_lock, NULL);

    while (1) {
        sem_wait(&session->session_sem);

        if (0) { //END
            break;
        }

        sscanf(session->req_pipe_path, "/tmp/%d_request", &session->id);

        session->req_pipe = open(session->req_pipe_path, O_RDONLY);
        session->notif_pipe = open(session->notif_pipe_path, O_WRONLY);

        char buf = '0';
        if (session->req_pipe == -1 || session->notif_pipe == -1) {
            end_session(session);
            buf = '1';
            continue;
        }

        char op_code = OP_CODE_CONNECT;
        if (write(session->notif_pipe, &op_code, 1) != 1 ||
            write(session->notif_pipe, &buf, 1) != 1) {
            perror("Error writing to notif pipe - connect");
            exit(EXIT_FAILURE);
        }

        int wait = run_game(session, manager.levels_dir);

        if (wait) {
            while (read_request_pipe(session) != 'Q') {}
        }

        close(session->req_pipe);
        close(session->notif_pipe);

        end_session(session);
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
    signal(SIGPIPE, SIG_IGN);

    manager.levels_dir = argv[1];
    manager.max_games = atoi(argv[2]);
    manager.server_pipe_path = argv[3];
    manager.ongoing_sessions = 0;
    sem_init(&manager.server_sem, 0, manager.max_games);
    pthread_mutex_init(&manager.server_lock, NULL);
    if (!(manager.all_sessions = calloc(manager.max_games, sizeof(Server_session)))) {
        perror("Error allocating memory to manager");
        exit(EXIT_FAILURE);
    }
    unlink(manager.server_pipe_path);
    if (mkfifo(manager.server_pipe_path, 0666) != 0) {
        perror("Error creating named pipe");
        exit(EXIT_FAILURE);
    }
    int server_pipe_fd = open(manager.server_pipe_path, O_RDWR);

    pthread_t session_tid[manager.max_games];

    for (int i = 0; i < manager.max_games; i++) {
        pthread_create(&session_tid[i], NULL, session_thread, &manager.all_sessions[i]);
    }


    while (1) {

        if (0) { //END
            break;
        }
        sem_wait(&manager.server_sem);
        change_ongoing_sessions(1);

        Server_session* session = find_open_session();

        char op_code;
        read_char(server_pipe_fd, &op_code, 1);

        if (op_code != OP_CODE_CONNECT) {
            perror("Error wrong op code in server pipe");
            exit(EXIT_FAILURE);
        }

        read_char(server_pipe_fd, session->req_pipe_path, MAX_PIPE_PATH_LENGTH);
        read_char(server_pipe_fd, session->notif_pipe_path, MAX_PIPE_PATH_LENGTH);

        sem_post(&session->session_sem);
    }


    for (int i = 0; i < manager.max_games; i++) {
        pthread_join(session_tid[i], NULL);
    }
    sem_destroy(&manager.server_sem);
    pthread_mutex_destroy(&manager.server_lock);
    free(manager.all_sessions);
    close(server_pipe_fd);
    unlink(manager.server_pipe_path);

    return 0;
}