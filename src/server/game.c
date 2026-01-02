#include "board.h"
#include "display.h"
#include "protocol.h"
#include "game.h"
#include "parser.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1
#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4

typedef struct {
    board_t* board;
    int ghost_idx;
} ghost_thread_args;

int pacman_alive(board_t* board) {
    pacman_t* pacman = &board->pacmans[0];
    pthread_mutex_lock(&pacman->pac_lock);
    int status = pacman->alive;
    pthread_mutex_unlock(&pacman->pac_lock);
    return status;
}

void send_board(Server_session* session) {
    board_t* board = &session->board;
    char op_code = OP_CODE_BOARD;
    if (write(session->notif_pipe, &op_code, sizeof(char)) != sizeof(char) ||
        write(session->notif_pipe, &board->width, sizeof(int)) != sizeof(int) ||
        write(session->notif_pipe, &board->height, sizeof(int)) != sizeof(int) ||
        write(session->notif_pipe, &board->tempo, sizeof(int)) != sizeof(int) ||
        write(session->notif_pipe, &board->victory, sizeof(int)) != sizeof(int) ||
        write(session->notif_pipe, &board->game_over, sizeof(int)) != sizeof(int) ||
        write(session->notif_pipe, &board->pacmans[0].points, sizeof(int)) != sizeof(int)) {

        perror("Error writing to request pipe - play");
        exit(EXIT_FAILURE);
    }
}

void *display_thread(void* arg) {
    Server_session* session = (Server_session*) arg;
    board_t* board = &session->board;
    
    sleep_ms(board->tempo / 2);

    while (1) {
        sleep_ms(board->tempo);

        pthread_rwlock_wrlock(&board->board_lock);
        if (board->shutdown_threads) {
            pthread_rwlock_unlock(&board->board_lock);
            pthread_exit(NULL);
        }

        send_board(session);

        pthread_rwlock_unlock(&board->board_lock);
    }
}

void *pacman_thread(void* arg) {
    Server_session* session = (Server_session*)arg;
    board_t* board = &session->board;

    int *play_result = malloc(sizeof(int));

    while (1) {
        if (!pacman_alive(board)) {
            *play_result = QUIT_GAME;
            return (void*) play_result;
        }

        command_t* play;
        command_t c;

        c.command = read_request_pipe(session);

        play = &c;

        debug("KEY %c\n", play->command);
        
        if (play->command == 'Q') {
            *play_result = QUIT_GAME;
            return (void*) play_result;
        }

        pthread_rwlock_rdlock(&board->board_lock);

        int result = move_pacman(board, 0, play);

        if (result == REACHED_PORTAL) {
            // Next level
            *play_result = NEXT_LEVEL;
            break;
        }

        if (result == DEAD_PACMAN) {
            *play_result = QUIT_GAME;
            break;
        }
        pthread_rwlock_unlock(&board->board_lock);
    }
    pthread_rwlock_unlock(&board->board_lock);

    return (void*) play_result;
}

void *ghost_thread(void* arg) {
    ghost_thread_args* args = (ghost_thread_args*)arg;
    board_t* board = args->board;
    int ghost_idx = args->ghost_idx;

    free(args);

    ghost_t* ghost = &board->ghosts[ghost_idx];

    while (1) {
        sleep_ms(board->tempo);

        pthread_rwlock_rdlock(&board->board_lock);

        if (board->shutdown_threads) {
            pthread_rwlock_unlock(&board->board_lock);
            pthread_exit(NULL);
        }
        
        move_ghost(board, ghost_idx, &ghost->moves[ghost->current_move%ghost->n_moves]);

        if (!pacman_alive(board)) {
            pthread_rwlock_unlock(&board->board_lock);
            pthread_exit(NULL);
        }
        pthread_rwlock_unlock(&board->board_lock);
    }
}

int play_board_threads(Server_session* session) {
    board_t* board = &session->board;

    pthread_t display_tid, pac_tid, ghost_tid[MAX_GHOSTS];

    board->shutdown_threads = 0;

    pthread_create(&display_tid, NULL, display_thread, session);
    pthread_create(&pac_tid, NULL, pacman_thread, session);
    for (int i = 0; i < board->n_ghosts; i++) {
        ghost_thread_args* args;
        if (!(args = calloc(1, sizeof(*args)))) {
            perror("Error allocating memory to ghost thread args");
            exit(EXIT_FAILURE);
        }

        args->board = board;
        args->ghost_idx = i;

        pthread_create(&ghost_tid[i], NULL, ghost_thread, args);
    }

    int *play_result;
    pthread_join(pac_tid, (void**)&play_result);

    pthread_rwlock_wrlock(&board->board_lock);
    board->shutdown_threads = 1;
    pthread_rwlock_unlock(&board->board_lock);

    pthread_join(display_tid, NULL);
    for (int i = 0; i < board->n_ghosts; i++) {
        pthread_join(ghost_tid[i], NULL);
    }

    int result = *play_result;
    free(play_result);

    return result;
}


int run_game(Server_session* session, const char* levels_dir) {

    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    open_debug_file("debug.log");
    
    int accumulated_points = 0;
    int end_game = 0;
    board_t* game_board = &session->board;
    
    strcpy(game_board->dir_name, levels_dir);

    DIR* dir = opendir(game_board->dir_name);
    if (!dir) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    while (!end_game && (entry = readdir(dir)) != NULL) {
        int len = strlen(entry->d_name);
        if (len <= 4 || strcmp(entry->d_name + len - 4, LEVEL) != 0) continue;

        load_level(game_board, entry->d_name);
        load_pacman(game_board,accumulated_points);
        load_ghosts(game_board);

        while (1) {

            int result = play_board_threads(session);

            if (result == NEXT_LEVEL) {
                game_board->victory = 1;
                send_board(session);
                sleep_ms(game_board->tempo);
                break;
            }

            if (result == QUIT_GAME) {
                sleep_ms(game_board->tempo);
                game_board->game_over = 1;
                send_board(session);
                end_game = 1;
                break;
            }
        }

    accumulated_points = game_board->pacmans[0].points;

    print_board(game_board);
    unload_level(game_board);

    }

    closedir(dir);


    close_debug_file();

    return 0;
}