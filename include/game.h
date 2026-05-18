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
    GameState   state;
    int         time_left;
    int         mission_index;

    int         fd_led;
    int         fd_fnd;
    int         fd_dot;
    int         fd_dip;
    int         fd_push;

    struct timespec last_tick;
    struct timespec last_frame;
    int         fire_frame;
} GameCtx;

int  game_init(GameCtx *ctx);
void game_destroy(GameCtx *ctx);
void game_update(GameCtx *ctx);

/* device helpers (game.c에 구현) */
int           dot_open(void);
void          dot_close(int fd);
void          dot_write_pattern(int fd, const unsigned char *pattern);
void          dot_clear(int fd);

int           led_open(void);
void          led_close(int fd);
void          led_write(int fd, unsigned char mask);

int           fnd_open(void);
void          fnd_close(int fd);
void          fnd_write_seconds(int fd, int sec);
void          fnd_write_mission(int fd, int mission_no, int sec);

int           push_open(void);
void          push_close(int fd);
unsigned char push_read(int fd);

int           dip_open(void);
void          dip_close(int fd);
unsigned char dip_read(int fd);

/* animations */
void anim_bomb_fire(GameCtx *ctx);
void anim_explosion(GameCtx *ctx);
void anim_success(GameCtx *ctx);

#endif /* GAME_H */
