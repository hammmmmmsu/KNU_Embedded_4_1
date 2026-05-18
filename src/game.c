#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include "config.h"
#include "game.h"
#include "dot_driver.h"
#include "../assets/dot_patterns.h"

/* ── device open/close ───────────────────────────────────── */

int dot_open(void)  { int fd = open(DEV_DOT,  O_WRONLY); if (fd<0) perror("dot");  return fd; }
int led_open(void)  { int fd = open(DEV_LED,  O_WRONLY); if (fd<0) perror("led");  return fd; }
int fnd_open(void)  { int fd = open(DEV_FND,  O_WRONLY); if (fd<0) perror("fnd");  return fd; }
int dip_open(void)  { int fd = open(DEV_DIP,  O_RDONLY); if (fd<0) perror("dip");  return fd; }
int push_open(void) { int fd = open(DEV_PUSH, O_RDONLY); if (fd<0) perror("push"); return fd; }

void dot_close(int fd)  { close(fd); }
void led_close(int fd)  { close(fd); }
void fnd_close(int fd)  { close(fd); }
void dip_close(int fd)  { close(fd); }
void push_close(int fd) { close(fd); }

/* ── device write/read helpers ───────────────────────────── */

void dot_write_pattern(int fd, const unsigned char *pat)
{
    write(fd, pat, DOT_ROW_COUNT);
}

void dot_clear(int fd)
{
    static const unsigned char blank[DOT_ROW_COUNT] = {0};
    write(fd, blank, DOT_ROW_COUNT);
}

void led_write(int fd, unsigned char mask)
{
    write(fd, &mask, 1);
}

/* FND: 4자리 BCD (digit[0]=천, digit[3]=일) */
void fnd_write_seconds(int fd, int sec)
{
    unsigned char d[4];
    if (sec < 0) sec = 0;
    d[0] = 0x0F;                       /* blank */
    d[1] = 0x0F;                       /* blank */
    d[2] = (unsigned char)((sec / 10) % 10);
    d[3] = (unsigned char)(sec % 10);
    write(fd, d, 4);
}

void fnd_write_mission(int fd, int mission_no, int sec)
{
    unsigned char d[4];
    d[0] = (unsigned char)(mission_no & 0x0F); /* 미션 번호 */
    d[1] = 0x0F;                               /* blank */
    d[2] = (unsigned char)((sec / 10) % 10);
    d[3] = (unsigned char)(sec % 10);
    write(fd, d, 4);
}

/* push: 값 읽기 — char device는 lseek 안 됨, 그냥 read */
unsigned char push_read(int fd)
{
    unsigned char val = 0;
    read(fd, &val, 1);
    return val;
}

unsigned char dip_read(int fd)
{
    unsigned char val = 0;
    read(fd, &val, 1);
    return val;
}

/* ── elapsed helpers ─────────────────────────────────────── */

static void ts_now(struct timespec *ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}

static long ts_elapsed_ms(const struct timespec *start)
{
    struct timespec now;
    ts_now(&now);
    return (now.tv_sec  - start->tv_sec)  * 1000L
         + (now.tv_nsec - start->tv_nsec) / 1000000L;
}

/* ── terminal display helpers ────────────────────────────── */

static void print_banner(void)
{
    printf("\033[2J\033[H"); /* clear screen */
    printf("╔══════════════════════════════════╗\n");
    printf("║       💣  BOMB  GAME  💣         ║\n");
    printf("╚══════════════════════════════════╝\n\n");
}

static void print_bits(unsigned char v)
{
    int i;
    for (i = 7; i >= 0; i--)
        printf("%c", (v >> i) & 1 ? '1' : '0');
}

/* ── button debounce (30ms) ──────────────────────────────── */

static int btn_pressed(int fd)
{
    unsigned char v = push_read(fd);
    if (v == 0) return 0;
    usleep(30000);
    v = push_read(fd);
    return v != 0;
}

static void wait_btn_release(int fd)
{
    while (push_read(fd) != 0)
        usleep(10000);
}

/* ── fire animation (non-blocking) ──────────────────────── */

void anim_bomb_fire(GameCtx *ctx)
{
    if (ts_elapsed_ms(&ctx->last_frame) < FIRE_FRAME_MS)
        return;
    ctx->fire_frame = (ctx->fire_frame + 1) % DOT_FIRE_FRAME_COUNT;
    dot_write_pattern(ctx->fd_dot, DOT_BOMB_FIRE[ctx->fire_frame]);
    led_write(ctx->fd_led, (ctx->fire_frame % 2) ? LED_ALL_ON : LED_ALL_OFF);
    ts_now(&ctx->last_frame);
}

/* ── explosion animation (blocking) ─────────────────────── */

void anim_explosion(GameCtx *ctx)
{
    int i, j;
    printf("\n💥  BOOM!  💥\n\n");
    for (i = 0; i < DOT_EXPLOSION_FRAME_COUNT; i++) {
        dot_write_pattern(ctx->fd_dot, DOT_EXPLOSION[i]);
        led_write(ctx->fd_led,
                  (DOT_EXPLOSION[i][0] == 0x7F) ? LED_ALL_ON : LED_ALL_OFF);
        usleep((unsigned int)DOT_EXPLOSION_DELAY_MS[i] * 1000);
    }
    for (j = 0; j < 6; j++) {
        led_write(ctx->fd_led, j % 2 ? LED_ALL_ON : LED_ALL_OFF);
        usleep(120000);
    }
    led_write(ctx->fd_led, LED_ALL_OFF);
    dot_clear(ctx->fd_dot);
}

/* ── success animation (blocking) ───────────────────────── */

void anim_success(GameCtx *ctx)
{
    static const unsigned char sp[] = DOT_SUCCESS;
    int j;
    dot_write_pattern(ctx->fd_dot, sp);
    for (j = 0; j < 4; j++) {
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(250000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        usleep(150000);
    }
    led_write(ctx->fd_led, LED_ALL_ON);
}

/* ═══════════════════════════════════════════════════════════
 *  MISSION: DIP switch 패턴 맞추기
 *
 *  - LED로 목표 패턴을 보여줌
 *  - 사용자가 DIP 스위치를 같은 패턴으로 설정
 *  - SW1(인터럽트 버튼) 눌러서 제출
 *  - 맞으면 통과, 틀리면 -5초 페널티 후 재도전
 * ═══════════════════════════════════════════════════════════ */

static unsigned char mission_targets[MISSION_COUNT];

static void mission_show(GameCtx *ctx, int idx)
{
    printf("\n┌─────────────────────────────┐\n");
    printf("│  미션 %d / %d                 │\n", idx + 1, MISSION_COUNT);
    printf("│  남은 시간: %2d초             │\n", ctx->time_left);
    printf("│                             │\n");
    printf("│  LED 패턴:  ");
    print_bits(mission_targets[idx]);
    printf("  │\n");
    printf("│  DIP 스위치를 맞추고        │\n");
    printf("│  SW1(인터럽트) 버튼 누르기  │\n");
    printf("└─────────────────────────────┘\n");
    led_write(ctx->fd_led, mission_targets[idx]);
}

/* mission 진행 — 1 step씩 호출됨. 완료 시 1 반환 */
static int mission_step(GameCtx *ctx, int idx)
{
    static int last_show = -1;   /* 마지막으로 출력한 미션 인덱스 */
    unsigned char dip;

    /* 미션이 바뀌었을 때만 화면 출력 */
    if (last_show != idx) {
        mission_show(ctx, idx);
        last_show = idx;
    }

    dip = dip_read(ctx->fd_dip);

    /* 현재 DIP 상태 한 줄 갱신 */
    printf("\r  현재 DIP:  ");
    print_bits(dip);
    if (dip == mission_targets[idx])
        printf("  ✓ 일치! SW1 눌러서 제출  ");
    else
        printf("  ✗ 불일치              ");
    fflush(stdout);

    /* SW1 누름 감지 */
    if (!btn_pressed(ctx->fd_push))
        return 0;

    wait_btn_release(ctx->fd_push);

    if (dip == mission_targets[idx]) {
        printf("\n\n  [OK] 미션 %d 해제 성공!\n", idx + 1);
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(400000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        last_show = -1;   /* 다음 미션 때 다시 출력 */
        return 1;
    } else {
        printf("\n\n  [!!] 틀렸습니다! -5초 페널티\n");
        /* 경고 LED 점멸 */
        int j;
        for (j = 0; j < 4; j++) {
            led_write(ctx->fd_led, LED_ALL_ON);
            usleep(100000);
            led_write(ctx->fd_led, LED_ALL_OFF);
            usleep(100000);
        }
        ctx->time_left -= 5;
        if (ctx->time_left < 0) ctx->time_left = 0;
        /* 패턴 다시 표시 */
        mission_show(ctx, idx);
        return 0;
    }
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

    if (ctx->fd_led<0 || ctx->fd_fnd<0 || ctx->fd_dot<0 ||
        ctx->fd_dip<0 || ctx->fd_push<0)
        return -1;

    ctx->state      = STATE_WAITING;
    ctx->time_left  = GAME_TIME_SEC;
    ctx->fire_frame = 0;
    ts_now(&ctx->last_frame);
    ts_now(&ctx->last_tick);

    led_write(ctx->fd_led, LED_ALL_OFF);
    fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
    dot_write_pattern(ctx->fd_dot, DOT_BOMB_FIRE[0]);

    print_banner();
    printf("  SW1(인터럽트 버튼)을 눌러 게임을 시작하세요.\n\n");
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

/* ── game_update (메인 루프에서 매 10ms 호출) ─────────────── */

void game_update(GameCtx *ctx)
{
    switch (ctx->state) {

    /* ── WAITING ─────────────────────────────────────── */
    case STATE_WAITING:
        anim_bomb_fire(ctx);
        fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);

        if (btn_pressed(ctx->fd_push)) {
            wait_btn_release(ctx->fd_push);

            /* 랜덤 미션 패턴 생성 (0 제외) */
            srand((unsigned int)time(NULL));
            int i;
            for (i = 0; i < MISSION_COUNT; i++)
                mission_targets[i] = (unsigned char)(rand() % 255 + 1);

            ctx->state         = STATE_PLAYING;
            ctx->time_left     = GAME_TIME_SEC;
            ctx->mission_index = 0;
            ts_now(&ctx->last_tick);
            ts_now(&ctx->last_frame);

            print_banner();
            printf("  ██ 게임 시작! 폭탄을 해제하세요! ██\n");
            printf("  총 %d개의 미션을 %d초 안에 완료!\n\n",
                   MISSION_COUNT, GAME_TIME_SEC);
            led_write(ctx->fd_led, LED_ALL_OFF);
        }
        break;

    /* ── PLAYING ─────────────────────────────────────── */
    case STATE_PLAYING: {
        /* 1초마다 카운트다운 */
        long e = ts_elapsed_ms(&ctx->last_tick);
        if (e >= 1000) {
            ctx->time_left -= (int)(e / 1000);
            ts_now(&ctx->last_tick);
            fnd_write_mission(ctx->fd_fnd, ctx->mission_index + 1,
                              ctx->time_left);
            if (ctx->time_left <= 0) {
                ctx->state = STATE_FAIL;
                break;
            }
        }

        /* DOT: 폭탄 불꽃 유지 */
        anim_bomb_fire(ctx);

        /* 현재 미션 진행 */
        if (mission_step(ctx, ctx->mission_index)) {
            ctx->mission_index++;
            if (ctx->mission_index >= MISSION_COUNT)
                ctx->state = STATE_SUCCESS;
        }
        break;
    }

    /* ── SUCCESS ─────────────────────────────────────── */
    case STATE_SUCCESS:
        print_banner();
        printf("  ★★★  폭탄 해제 성공!  ★★★\n\n");
        printf("  남은 시간: %d초\n\n", ctx->time_left);
        anim_success(ctx);
        fnd_write_seconds(ctx->fd_fnd, ctx->time_left);
        sleep(3);

        /* 대기 상태로 복귀 */
        ctx->state      = STATE_WAITING;
        ctx->time_left  = GAME_TIME_SEC;
        ctx->mission_index = 0;
        led_write(ctx->fd_led, LED_ALL_OFF);
        fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
        print_banner();
        printf("  SW1(인터럽트 버튼)을 눌러 게임을 시작하세요.\n\n");
        ts_now(&ctx->last_frame);
        break;

    /* ── FAIL ────────────────────────────────────────── */
    case STATE_FAIL:
        print_banner();
        printf("  ▓▓▓  시간 초과 — 폭발!  ▓▓▓\n\n");
        anim_explosion(ctx);
        fnd_write_seconds(ctx->fd_fnd, 0);
        sleep(2);

        ctx->state      = STATE_WAITING;
        ctx->time_left  = GAME_TIME_SEC;
        ctx->mission_index = 0;
        fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
        print_banner();
        printf("  SW1(인터럽트 버튼)을 눌러 게임을 시작하세요.\n\n");
        ts_now(&ctx->last_frame);
        break;
    }
}
