#ifndef GAME_H
#define GAME_H

#include "protocol.h"

typedef struct {
    int id;
    int req_pipe;
    int notif_pipe;
    char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    int running; // 0 if stopped, 1 if active
    sem_t session_sem;
    pthread_mutex_t session_lock;
    board_t board;
} Server_session;


typedef struct {
    const char* levels_dir;
    int max_games;
    const char* server_pipe_path;
    int ongoing_sessions;
    sem_t server_sem;
    pthread_mutex_t server_lock;
    Server_session* all_sessions;
} Server_manager;

int run_game(Server_session* session, char levels_dir);

#endif