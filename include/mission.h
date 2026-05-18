#ifndef MISSION_H
#define MISSION_H

#include "config.h"
#include "game.h"

typedef enum {
    MISSION_DIP_PATTERN = 0,   /* match DIP switch to shown LED pattern */
    MISSION_BUTTON_TIMING,     /* press button within time window */
    MISSION_LED_MEMORY,        /* remember LED sequence and reproduce */
} MissionType;

typedef struct {
    MissionType type;
    int         complete;
    unsigned char target;      /* target pattern for DIP / LED missions */
} Mission;

extern Mission missions[MISSION_COUNT];

/* Run one step of the current mission. Returns 1 if mission completed. */
int mission_update(GameCtx *ctx, Mission *m);

/* Setup mission at start */
void mission_init(Mission *m, int index);

#endif /* MISSION_H */
