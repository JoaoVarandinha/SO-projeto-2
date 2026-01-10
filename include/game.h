#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "protocol.h"
#include <semaphore.h>

typedef struct {
    int id;
    int req_pipe;
    int notif_pipe;
    char req_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    char notif_pipe_path[MAX_PIPE_PATH_LENGTH + 1];
    int running; // 0 if stopped, 1 if active
    pthread_mutex_t session_lock;
    board_t board;
} Server_session;


typedef struct {
    const char* levels_dir;
    int max_games;
    const char* server_pipe_path;
    Server_session* all_sessions;
} Server_manager;

typedef struct {
    char req_pipe_path[MAX_PIPE_NAME];
    char notif_pipe_path[MAX_PIPE_NAME];
} Connect_request;

typedef struct {
    sem_t empty_sem;
    sem_t available_sem;
    int ongoing_sessions;
    Connect_request req;
    pthread_mutex_t buf_lock;
} Server_pipe_buf;


int run_game(Server_session* session, const char* levels_dir);

#endif