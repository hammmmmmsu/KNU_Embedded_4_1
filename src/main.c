#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "game.h"
#include "config.h"

static volatile int running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    GameCtx ctx;

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    srand((unsigned int)time(NULL));

    printf("=== Bomb Game Start ===\n");
    printf("Press SW1 to start. Ctrl-C to quit.\n");

    if (game_init(&ctx) < 0) {
        fprintf(stderr, "Failed to open devices. Are drivers loaded?\n");
        return EXIT_FAILURE;
    }

    /* Main loop: ~10 ms polling interval */
    while (running) {
        game_update(&ctx);
        usleep(10000);
    }

    printf("\nShutting down...\n");
    game_destroy(&ctx);
    return EXIT_SUCCESS;
}
