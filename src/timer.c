/*
 * timer.c — game countdown helpers.
 * The main countdown tick is handled in game_update(); these are utilities.
 */
#include <time.h>
#include "game.h"

/* Returns milliseconds since *start without modifying it */
long timer_elapsed_ms(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec  - start->tv_sec)  * 1000L
         + (now.tv_nsec - start->tv_nsec) / 1000000L;
}

void timer_reset(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}
