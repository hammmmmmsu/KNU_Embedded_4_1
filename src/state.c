/*
 * state.c — state transition helpers.
 * Actual transitions happen inside game_update() in game.c;
 * this file provides logging / hook points.
 */
#include <stdio.h>
#include "game.h"

static const char *state_name(GameState s)
{
    switch (s) {
    case STATE_WAITING: return "WAITING";
    case STATE_PLAYING: return "PLAYING";
    case STATE_SUCCESS: return "SUCCESS";
    case STATE_FAIL:    return "FAIL";
    default:            return "UNKNOWN";
    }
}

void state_transition(GameCtx *ctx, GameState next)
{
    if (ctx->state == next) return;
    printf("[state] %s -> %s\n", state_name(ctx->state), state_name(next));
    ctx->state = next;
}
