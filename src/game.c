#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "game.h"
#include "dot_driver.h"
#include "led_driver.h"
#include "fnd_driver.h"
#include "interrupt_driver.h"
#include "dip_driver.h"
#include "mission.h"
#include "../assets/dot_patterns.h"

/* ── helpers ──────────────────────────────────────────────── */

static long ms_elapsed(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec  - start->tv_sec)  * 1000L
         + (now.tv_nsec - start->tv_nsec) / 1000000L;
}

static void timestamp(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

/* ── device wrappers ──────────────────────────────────────── */

int dot_open(void)
{
    int fd = open(DEV_DOT, O_WRONLY);
    if (fd < 0) perror("dot_open");
    return fd;
}

void dot_close(int fd)   { close(fd); }

void dot_write_pattern(int fd, const unsigned char *pattern)
{
    write(fd, pattern, DOT_ROW_COUNT);
}

void dot_clear(int fd)
{
    static const unsigned char blank[DOT_ROW_COUNT] = {0};
    dot_write_pattern(fd, blank);
}

int led_open(void)
{
    int fd = open(DEV_LED, O_WRONLY);
    if (fd < 0) perror("led_open");
    return fd;
}

void led_close(int fd)   { close(fd); }

void led_write(int fd, unsigned char mask)
{
    write(fd, &mask, 1);
}

int fnd_open(void)
{
    int fd = open(DEV_FND, O_WRONLY);
    if (fd < 0) perror("fnd_open");
    return fd;
}

void fnd_close(int fd)   { close(fd); }

void fnd_write_seconds(int fd, int seconds)
{
    unsigned char digits[4];
    if (seconds < 0) seconds = 0;
    digits[0] = 0x0F;                    /* blank */
    digits[1] = 0x0F;                    /* blank */
    digits[2] = (unsigned char)((seconds / 10) % 10);
    digits[3] = (unsigned char)(seconds % 10);
    write(fd, digits, 4);
}

void fnd_write_digits(int fd,
                      unsigned char d0, unsigned char d1,
                      unsigned char d2, unsigned char d3)
{
    unsigned char digits[4] = {d0, d1, d2, d3};
    write(fd, digits, 4);
}

int push_open(void)
{
    int fd = open(DEV_PUSH, O_RDONLY);
    if (fd < 0) perror("push_open");
    return fd;
}

void push_close(int fd)   { close(fd); }

unsigned char push_read(int fd)
{
    unsigned char val = 0;
    lseek(fd, 0, SEEK_SET);
    read(fd, &val, 1);
    return val;
}

int push_pressed(int fd, unsigned char btn_mask)
{
    return (push_read(fd) & btn_mask) ? 1 : 0;
}

int dip_open(void)
{
    int fd = open(DEV_DIP, O_RDONLY);
    if (fd < 0) perror("dip_open");
    return fd;
}

void dip_close(int fd)   { close(fd); }

unsigned char dip_read(int fd)
{
    unsigned char val = 0;
    lseek(fd, 0, SEEK_SET);
    read(fd, &val, 1);
    return val;
}

/* ── game_init / game_destroy ────────────────────────────── */

int game_init(GameCtx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->fd_led  = led_open();
    ctx->fd_fnd  = fnd_open();
    ctx->fd_dot  = dot_open();
    ctx->fd_dip  = dip_open();
    ctx->fd_push = push_open();

    if (ctx->fd_led < 0 || ctx->fd_fnd < 0 || ctx->fd_dot  < 0 ||
        ctx->fd_dip < 0 || ctx->fd_push < 0)
        return -1;

    ctx->state        = STATE_WAITING;
    ctx->time_left    = GAME_TIME_SEC;
    ctx->mission_index = 0;
    ctx->fire_frame   = 0;

    timestamp(&ctx->last_tick);
    timestamp(&ctx->last_frame);

    /* Initial display */
    led_write(ctx->fd_led, LED_ALL_OFF);
    fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
    dot_write_pattern(ctx->fd_dot, DOT_BOMB_FIRE[0]);

    return 0;
}

void game_destroy(GameCtx *ctx)
{
    led_write(ctx->fd_led, LED_ALL_OFF);
    dot_clear(ctx->fd_dot);

    led_close(ctx->fd_led);
    fnd_close(ctx->fd_fnd);
    dot_close(ctx->fd_dot);
    dip_close(ctx->fd_dip);
    push_close(ctx->fd_push);
}

/* ── animations ──────────────────────────────────────────── */

/*
 * anim_bomb_fire: cycle fire frames every FIRE_FRAME_MS milliseconds.
 * Also blinks the LED bar slowly (all on/off every ~500ms).
 */
void anim_bomb_fire(GameCtx *ctx)
{
    long elapsed = ms_elapsed(&ctx->last_frame);
    if (elapsed < FIRE_FRAME_MS)
        return;

    ctx->fire_frame = (ctx->fire_frame + 1) % DOT_FIRE_FRAME_COUNT;
    dot_write_pattern(ctx->fd_dot, DOT_BOMB_FIRE[ctx->fire_frame]);

    /* Slow LED blink: on for even frames, off for odd */
    led_write(ctx->fd_led,
              (ctx->fire_frame % 2 == 0) ? LED_ALL_ON : LED_ALL_OFF);

    timestamp(&ctx->last_frame);
}

/*
 * anim_explosion: plays the full explosion sequence and sets state to FAIL.
 * Blocking — runs the whole animation then returns.
 */
void anim_explosion(GameCtx *ctx)
{
    int i, j;

    for (i = 0; i < DOT_EXPLOSION_FRAME_COUNT; i++) {
        dot_write_pattern(ctx->fd_dot, DOT_EXPLOSION[i]);

        /* On flash frames (all-on), also blast the LEDs */
        if (DOT_EXPLOSION[i][0] == 0x7F) {
            led_write(ctx->fd_led, LED_ALL_ON);
        } else {
            led_write(ctx->fd_led, LED_ALL_OFF);
        }

        usleep((unsigned int)DOT_EXPLOSION_DELAY_MS[i] * 1000);
    }

    /* Final rapid LED flash x5 then all off — "사라지는" 효과 */
    for (j = 0; j < 5; j++) {
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(100000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        usleep(100000);
    }

    dot_clear(ctx->fd_dot);
    led_write(ctx->fd_led, LED_ALL_OFF);
    fnd_write_digits(ctx->fd_fnd, 0x0F, 0x0F, 0x0F, 0x0F); /* blank FND */
}

/*
 * anim_success: heart pattern on DOT, all LEDs on, FND shows "good".
 */
void anim_success(GameCtx *ctx)
{
    static const unsigned char success_pat[] = DOT_SUCCESS;
    int j;

    dot_write_pattern(ctx->fd_dot, success_pat);

    /* Three celebratory LED flashes */
    for (j = 0; j < 3; j++) {
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(300000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        usleep(200000);
    }
    led_write(ctx->fd_led, LED_ALL_ON);
    fnd_write_digits(ctx->fd_fnd, 0x0, 0x0, 0x0, 0x0); /* "0000" */
}

/* ── game_update ─────────────────────────────────────────── */

void game_update(GameCtx *ctx)
{
    switch (ctx->state) {

    /* ── WAITING ─────────────────────────────────────── */
    case STATE_WAITING:
        anim_bomb_fire(ctx);
        fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);

        if (push_pressed(ctx->fd_push, BTN_START)) {
            usleep(200000); /* debounce */
            ctx->state       = STATE_PLAYING;
            ctx->time_left   = GAME_TIME_SEC;
            ctx->mission_index = 0;
            mission_init(&missions[0], 0);
            timestamp(&ctx->last_tick);
            timestamp(&ctx->last_frame);
            led_write(ctx->fd_led, LED_ALL_OFF);
        }
        break;

    /* ── PLAYING ─────────────────────────────────────── */
    case STATE_PLAYING: {
        /* Countdown */
        long elapsed_sec = ms_elapsed(&ctx->last_tick) / 1000;
        if (elapsed_sec >= 1) {
            ctx->time_left -= (int)elapsed_sec;
            timestamp(&ctx->last_tick);
            fnd_write_seconds(ctx->fd_fnd, ctx->time_left);

            if (ctx->time_left <= 0) {
                ctx->state = STATE_FAIL;
                break;
            }
        }

        /* Run current mission */
        if (ctx->mission_index < MISSION_COUNT) {
            Mission *m = &missions[ctx->mission_index];
            if (mission_update(ctx, m)) {
                ctx->mission_index++;
                if (ctx->mission_index < MISSION_COUNT)
                    mission_init(&missions[ctx->mission_index],
                                 ctx->mission_index);
                else
                    ctx->state = STATE_SUCCESS;
            }
        }
        break;
    }

    /* ── SUCCESS ─────────────────────────────────────── */
    case STATE_SUCCESS:
        anim_success(ctx);
        sleep(3);
        ctx->state = STATE_WAITING;
        ctx->time_left = GAME_TIME_SEC;
        ctx->mission_index = 0;
        timestamp(&ctx->last_frame);
        break;

    /* ── FAIL ────────────────────────────────────────── */
    case STATE_FAIL:
        anim_explosion(ctx);
        sleep(2);
        ctx->state = STATE_WAITING;
        ctx->time_left = GAME_TIME_SEC;
        ctx->mission_index = 0;
        timestamp(&ctx->last_frame);
        break;
    }
}
