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

#define HIGHSCORE_FILE "info_client_files/points.txt"

static Server_manager manager;

volatile sig_atomic_t sigusr1_flag = 0;

void sigusr1_handler(int signum) {
    (void)signum;
    sigusr1_flag = 1;
}

void print_current_highscores(int current_sessions) {
    sigusr1_flag = 0;
    int picked[manager.max_games];
    for (int i = 0; i < manager.max_games; i++) picked[i] = 0;

    int amount_sessions = current_sessions < 5 ? current_sessions : 5;

    char buf[MAX_INSTRUCTION_LENGTH] = "";

    for (int i = 0; i < amount_sessions; i++) {
        int max_pos = -1, max_points = -1, max_id;

        for (int j = 0; j < manager.max_games; j++) {
            Server_session* ses = &manager.all_sessions[j];
            pthread_rwlock_wrlock(&ses->board.board_lock);
            if (!ses->running || picked[j]) continue;

            int pts = ses->board.pacmans[0].points;
            if (max_points >= pts) continue;

            max_points = pts;
            max_id = ses->id;
            max_pos = j;
        }
        if (max_pos == -1) break;

        picked[max_pos] = 1;

        char temp[40];
        sprintf(temp, "Id:%d - Points:%d\n", max_id, max_points);
        strcat(buf, temp);
    }

    for (int i = 0; i < manager.max_games; i++) {
        pthread_rwlock_unlock(&manager.all_sessions[i].board.board_lock);
    }

    int high_fd = open(HIGHSCORE_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (high_fd == -1 || write(high_fd, buf, strlen(buf)) != (ssize_t)strlen(buf)) {
        perror("Error writing to points file");
        exit(EXIT_FAILURE);
    }
    close(high_fd);
}

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
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
        perror("Error blocking SIGUSR1 in threads");
        exit(EXIT_FAILURE);
    }

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
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

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

        if (sigusr1_flag) print_current_highscores(check_ongoing_sessions()); //SIGUSR1 FLAG

        while (sem_trywait(&manager.server_sem) != 0) {
            sleep_ms(500);
            if (sigusr1_flag) print_current_highscores(check_ongoing_sessions()); //SIGUSR1 FLAG
        }
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