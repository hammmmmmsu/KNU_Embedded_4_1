#ifndef GAME_H
#define GAME_H

#include <time.h>

typedef enum {
    STATE_WAITING = 0,
    STATE_PLAYING,
    STATE_SUCCESS,
    STATE_FAIL,
} GameState;

typedef struct {
    GameState    state;
    int          time_left;      /* seconds remaining */
    int          mission_index;  /* 0-based current mission */
    int          mission_done;   /* missions completed */

    int          fd_led;
    int          fd_fnd;
    int          fd_dot;
    int          fd_dip;
    int          fd_push;

    struct timespec last_tick;   /* for countdown */
    struct timespec last_frame;  /* for animation */
    int          fire_frame;     /* current fire animation index */
} GameCtx;

/* Initialise / teardown */
int  game_init(GameCtx *ctx);
void game_destroy(GameCtx *ctx);

/* Main state-machine step — call in a tight loop */
void game_update(GameCtx *ctx);

/* Animations called from game_update */
void anim_bomb_fire(GameCtx *ctx);
void anim_explosion(GameCtx *ctx);
void anim_success(GameCtx *ctx);

#endif /* GAME_H */
