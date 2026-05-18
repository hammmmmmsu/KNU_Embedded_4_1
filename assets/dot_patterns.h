#ifndef DOT_PATTERNS_H
#define DOT_PATTERNS_H

/*
 * DOT Matrix patterns: 10 rows x 7 columns
 * Each byte = one row. Bits 6:0 represent columns left to right.
 * bit6 = leftmost col, bit0 = rightmost col
 *
 * Example: 0x3E = 0b0111110 = .11111.
 *          0x7F = 0b1111111 = 1111111
 *          0x08 = 0b0001000 = ...1...  (center)
 */

/* ── Clear (all off) ──────────────────────────── */
#define DOT_CLEAR \
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

/* ── Static bomb (no fire, used as base shape) ───
 * Row 0: ...1...  fuse top
 * Row 1: ...1...  fuse
 * Row 2: ..111..  bomb shoulder
 * Row 3: .11111.  bomb upper
 * Row 4: 1111111  bomb middle
 * Row 5: 1111111  bomb middle
 * Row 6: .11111.  bomb lower
 * Row 7: ..111..  bomb bottom
 * Row 8: .......
 * Row 9: .......
 */
#define DOT_BOMB_STATIC \
    { 0x08, 0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00, 0x00 }

/* ── Fire animation: 4 frames ────────────────────
 * Bomb body stays at rows 2-7; rows 0-1 show flickering flame.
 *
 * Frame 0: small spark (fuse dot only)
 * Frame 1: flame leaning left
 * Frame 2: flame tall center
 * Frame 3: flame leaning right
 */
#define DOT_FIRE_FRAME_COUNT 4
static const unsigned char DOT_BOMB_FIRE[DOT_FIRE_FRAME_COUNT][10] = {
    /* Frame 0 — small spark, center */
    { 0x08, 0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00, 0x00 },

    /* Frame 1 — flame leaning left (bit6 side)
     * Row 0: 11.....  (0x60)
     * Row 1: .11....  (0x30)  */
    { 0x60, 0x30, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00, 0x00 },

    /* Frame 2 — tall flame, center
     * Row 0: ..111..  (0x1C)
     * Row 1: ...1...  (0x08)  */
    { 0x1C, 0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00, 0x00 },

    /* Frame 3 — flame leaning right (bit0 side)
     * Row 0: .....11  (0x03)
     * Row 1: ....11.  (0x06)  */
    { 0x03, 0x06, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00, 0x00 },
};

/* ── Explosion animation: 7 frames ──────────────
 * Plays when game FAIL: burst → all-on flash → scatter → flash → ring → sparks → off
 */
#define DOT_EXPLOSION_FRAME_COUNT 7
static const unsigned char DOT_EXPLOSION[DOT_EXPLOSION_FRAME_COUNT][10] = {
    /* Frame 0 — center burst expanding */
    { 0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x00 },

    /* Frame 1 — ALL ON (first flash) */
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F },

    /* Frame 2 — checkerboard scatter
     * 0x55 = 1.1.1.1
     * 0x2A = .1.1.1.  */
    { 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A, 0x55, 0x2A },

    /* Frame 3 — ALL ON (second flash) */
    { 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F },

    /* Frame 4 — outer ring
     * 0x41 = 1.....1 */
    { 0x7F, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7F, 0x00 },

    /* Frame 5 — X sparks fading
     * 0x41 = 1.....1
     * 0x22 = .1...1.
     * 0x14 = ..1.1..
     * 0x08 = ...1...  */
    { 0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00 },

    /* Frame 6 — all off */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

/* Delay in ms for each explosion frame */
static const int DOT_EXPLOSION_DELAY_MS[DOT_EXPLOSION_FRAME_COUNT] = {
    150,  /* burst */
    250,  /* first flash */
    150,  /* scatter */
    250,  /* second flash */
    150,  /* ring */
    200,  /* sparks */
    500,  /* off (hold) */
};

/* ── Success pattern ─────────────────────────────
 * Heart shape: bomb defused!
 * Row 1: .11.11.  (0x36)
 * Row 2: 1111111  (0x7F)
 * Row 3: 1111111  (0x7F)
 * Row 4: .11111.  (0x3E)
 * Row 5: ..111..  (0x1C)
 * Row 6: ...1...  (0x08)
 */
#define DOT_SUCCESS \
    { 0x00, 0x36, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x00, 0x00 }

#endif /* DOT_PATTERNS_H */
