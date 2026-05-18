#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "config.h"
#include "game.h"
#include "mission.h"
#include "led_driver.h"
#include "dip_driver.h"
#include "interrupt_driver.h"
#include "fnd_driver.h"

Mission missions[MISSION_COUNT];

static unsigned char rand_pattern(void)
{
    /* Non-zero 8-bit pattern */
    unsigned char p = (unsigned char)(rand() % 255 + 1);
    return p;
}

void mission_init(Mission *m, int index)
{
    m->complete = 0;
    switch (index) {
    case 0:
        m->type   = MISSION_DIP_PATTERN;
        m->target = rand_pattern();
        break;
    case 1:
        m->type   = MISSION_BUTTON_TIMING;
        m->target = 0;
        break;
    case 2:
        m->type   = MISSION_LED_MEMORY;
        m->target = rand_pattern();
        break;
    default:
        m->type   = MISSION_DIP_PATTERN;
        m->target = rand_pattern();
    }
}

/* ── Mission 0: DIP switch pattern matching ────────────── */
static int mission_dip(GameCtx *ctx, Mission *m)
{
    /* Show target pattern on LEDs */
    led_write(ctx->fd_led, m->target);

    unsigned char dip = dip_read(ctx->fd_dip);
    if (dip == m->target) {
        /* Correct — brief green flash */
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(300000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        return 1;
    }
    return 0;
}

/* ── Mission 1: Press button before 3-second window ─────── */
static int mission_button(GameCtx *ctx, Mission *m)
{
    static struct timespec window_start = {0, 0};
    static int waiting = 0;

    if (!waiting) {
        /* Signal start: flash all LEDs 3 times */
        int i;
        for (i = 0; i < 3; i++) {
            led_write(ctx->fd_led, LED_ALL_ON);
            usleep(200000);
            led_write(ctx->fd_led, LED_ALL_OFF);
            usleep(200000);
        }
        clock_gettime(CLOCK_MONOTONIC, &window_start);
        waiting = 1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long elapsed = (now.tv_sec - window_start.tv_sec) * 1000L
                 + (now.tv_nsec - window_start.tv_nsec) / 1000000L;

    if (push_pressed(ctx->fd_push, BTN_START) && elapsed < 3000) {
        waiting = 0;
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(300000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        return 1;
    }

    if (elapsed >= 3000) {
        /* Missed window — penalise: subtract 10 seconds */
        ctx->time_left -= 10;
        if (ctx->time_left < 0) ctx->time_left = 0;
        fnd_write_seconds(ctx->fd_fnd, ctx->time_left);
        waiting = 0;
        /* Retry after brief pause */
        usleep(500000);
    }
    return 0;
}

/* ── Mission 2: Memorise LED sequence, reproduce with DIP ── */
static int mission_led_memory(GameCtx *ctx, Mission *m)
{
    static int shown = 0;

    if (!shown) {
        /* Show pattern for 1.5 s, then blank */
        led_write(ctx->fd_led, m->target);
        usleep(1500000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        shown = 1;
    }

    unsigned char dip = dip_read(ctx->fd_dip);
    if (push_pressed(ctx->fd_push, BTN_START)) {
        usleep(200000);
        if (dip == m->target) {
            shown = 0;
            led_write(ctx->fd_led, LED_ALL_ON);
            usleep(300000);
            led_write(ctx->fd_led, LED_ALL_OFF);
            return 1;
        } else {
            /* Wrong — penalise */
            ctx->time_left -= 5;
            if (ctx->time_left < 0) ctx->time_left = 0;
            fnd_write_seconds(ctx->fd_fnd, ctx->time_left);
            /* Re-show pattern */
            shown = 0;
        }
    }
    return 0;
}

int mission_update(GameCtx *ctx, Mission *m)
{
    switch (m->type) {
    case MISSION_DIP_PATTERN:   return mission_dip(ctx, m);
    case MISSION_BUTTON_TIMING: return mission_button(ctx, m);
    case MISSION_LED_MEMORY:    return mission_led_memory(ctx, m);
    default:                    return 0;
    }
}
