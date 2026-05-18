#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "config.h"
#include "game.h"
#include "dot_driver.h"
#include "../assets/dot_patterns.h"

/* ── device open/close ───────────────────────────────────── */

int dot_open(void)  { int fd = open(DEV_DOT,  O_WRONLY); if (fd<0) perror("dot");  return fd; }
int led_open(void)  { int fd = open(DEV_LED,  O_WRONLY); if (fd<0) perror("led");  return fd; }
int fnd_open(void)  { int fd = open(DEV_FND,  O_WRONLY); if (fd<0) perror("fnd");  return fd; }
int dip_open(void)  { int fd = open(DEV_DIP,  O_RDONLY);            if (fd<0) perror("dip");  return fd; }
int push_open(void) { int fd = open(DEV_PUSH, O_RDONLY|O_NONBLOCK); if (fd<0) perror("push"); return fd; }
int interrupt_open(void) { int fd = open(DEV_INTERRUPT, O_RDONLY); if (fd<0) perror("interrupt"); return fd; }

void dot_close(int fd)  { close(fd); }
void led_close(int fd)  { close(fd); }
void fnd_close(int fd)  { close(fd); }
void dip_close(int fd)  { close(fd); }
void push_close(int fd) { close(fd); }
void interrupt_close(int fd) { close(fd); }

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

/*
 * 현재 적재된 fnd_driver는 이전 실습 코드와 동일하게 packed BCD 2바이트를 받는다.
 * [천|백], [십|일] 형식으로 보내야 909 같은 깨진 값 대신 실제 숫자가 보인다.
 */
static void fnd_write_number(int fd, int value)
{
    unsigned char d[2];
    if (value < 0) value = 0;
    if (value > 9999) value = 9999;
    d[0] = (unsigned char)((((value / 1000) % 10) << 4) | ((value / 100) % 10));
    d[1] = (unsigned char)((((value / 10) % 10) << 4) | (value % 10));
    write(fd, d, sizeof(d));
}

void fnd_write_seconds(int fd, int sec)
{
    fnd_write_number(fd, sec);
}

void fnd_write_mission(int fd, int mission_no, int sec)
{
    (void)mission_no;
    fnd_write_seconds(fd, sec);
}

/* push_switch 읽기 (O_NONBLOCK — 안 눌리면 EAGAIN → 0 반환) */
unsigned char push_read(int fd)
{
    unsigned short val = 0;
    ssize_t n = read(fd, &val, sizeof(val));
    if (n < 0) return 0;   /* EAGAIN 포함 */
    return (unsigned char)(val & 0xFF);
}

unsigned char dip_read(int fd)
{
    unsigned char val = 0;
    ssize_t n = read(fd, &val, 1);
    if (n <= 0) return 0;
    return val;
}

unsigned int interrupt_read_count(int fd)
{
    unsigned int count = 0;
    ssize_t n = read(fd, &count, sizeof(count));
    lseek(fd, 0, SEEK_SET);
    if (n <= 0) return 0;
    return count;
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

static unsigned char reverse_bits8(unsigned char v)
{
    unsigned char out = 0;
    int i;
    for (i = 0; i < 8; i++) {
        out <<= 1;
        out |= (unsigned char)(v & 1);
        v >>= 1;
    }
    return out;
}

static void print_wire_pattern(unsigned char v)
{
    int i;
    for (i = 7; i >= 0; i--)
        printf("%c", (v >> i) & 1 ? '#' : '-');
}

/* ── button debounce (30ms) ──────────────────────────────── */

/* 1초마다 push 값 출력 (어떤 버튼 누르면 숫자 바뀌는지 확인용) */
static unsigned int last_interrupt_count = 0;

static int yellow_button_pressed(GameCtx *ctx)
{
    unsigned int now = interrupt_read_count(ctx->fd_interrupt);
    if (now == last_interrupt_count)
        return 0;
    last_interrupt_count = now;
    return 1;
}

static void yellow_diag_print(GameCtx *ctx)
{
    static struct timespec last = {0, 0};
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long diff = (now.tv_sec - last.tv_sec) * 1000L
              + (now.tv_nsec - last.tv_nsec) / 1000000L;
    if (diff < 1000) return;
    last = now;
    printf("\r  노란 버튼 카운트: %u   검은 버튼 raw: 0x%02X   ",
           interrupt_read_count(ctx->fd_interrupt), push_read(ctx->fd_push));
    fflush(stdout);
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
 *  MISSIONS
 *  1) DIP code match: LED target pattern을 DIP으로 맞춘 뒤 제출
 *  2) SW sequence: 터미널에 제시된 SW8~SW16 순서를 입력
 *  3) Mixed unlock: DIP 패턴을 맞춘 뒤 지정된 SW 버튼으로 최종 승인
 * ═══════════════════════════════════════════════════════════ */

typedef enum {
    MISSION_DIP_CODE = 0,
    MISSION_SW_SEQUENCE,
    MISSION_MIXED_UNLOCK,
} MissionKind;

typedef struct {
    MissionKind kind;
    unsigned char dip_target;
    unsigned short sw_sequence[3];
    int sequence_len;
    int sequence_pos;
    unsigned short confirm_button;
} MissionSpec;

static MissionSpec mission_specs[MISSION_COUNT];
static int mission_last_show = -1;
static unsigned short prev_push_bits = 0;

static const char *sw_code(unsigned short mask)
{
    switch (mask) {
    case 0x001: return "A";
    case 0x002: return "B";
    case 0x004: return "C";
    case 0x008: return "D";
    case 0x010: return "E";
    case 0x020: return "F";
    case 0x040: return "G";
    case 0x080: return "H";
    case 0x100: return "I";
    default:    return "?";
    }
}

static unsigned short push_read_all(int fd)
{
    unsigned short val = 0;
    ssize_t n = read(fd, &val, sizeof(val));
    if (n <= 0) return 0;
    return val;
}

static unsigned short push_new_press(int fd)
{
    unsigned short now = push_read_all(fd);
    unsigned short fresh = now & (unsigned short)(~prev_push_bits);
    prev_push_bits = now;
    return fresh;
}

static unsigned char led_for_push(unsigned short mask)
{
    switch (mask) {
    case 0x001: return 0x80;
    case 0x002: return 0x40;
    case 0x004: return 0x20;
    case 0x008: return 0x10;
    case 0x010: return 0x08;
    case 0x020: return 0x04;
    case 0x040: return 0x02;
    case 0x080: return 0x01;
    default:    return 0x00;
    }
}

static void seed_missions(void)
{
    static const unsigned short seq_pool[] = {0x002, 0x004, 0x008, 0x010, 0x020};
    int i;

    mission_specs[0].kind = MISSION_DIP_CODE;
    mission_specs[0].dip_target = (unsigned char)(rand() % 255 + 1);

    mission_specs[1].kind = MISSION_SW_SEQUENCE;
    mission_specs[1].sequence_len = 3;
    mission_specs[1].sequence_pos = 0;
    for (i = 0; i < mission_specs[1].sequence_len; i++)
        mission_specs[1].sw_sequence[i] = seq_pool[rand() % (sizeof(seq_pool) / sizeof(seq_pool[0]))];

    mission_specs[2].kind = MISSION_MIXED_UNLOCK;
    mission_specs[2].dip_target = (unsigned char)(rand() % 255 + 1);
    mission_specs[2].confirm_button = seq_pool[rand() % (sizeof(seq_pool) / sizeof(seq_pool[0]))];
}

static void mission_show(GameCtx *ctx, int idx)
{
    MissionSpec *m = &mission_specs[idx];
    printf("\n┌─────────────────────────────┐\n");
    printf("│  미션 %d / %d                 │\n", idx + 1, MISSION_COUNT);
    printf("│  남은 시간: %2d초             │\n", ctx->time_left);
    printf("│                             │\n");

    if (m->kind == MISSION_DIP_CODE) {
        printf("│  [WIRE CUT] 점등선 복제      │\n");
        printf("│  목표 선로: "); print_wire_pattern(reverse_bits8(m->dip_target)); printf("  │\n");
        printf("│  DIP 맞춘 뒤 노란 버튼 제출 │\n");
        led_write(ctx->fd_led, m->dip_target);
    } else if (m->kind == MISSION_SW_SEQUENCE) {
        printf("│  [AUTH] 버튼 코드 입력       │\n");
        printf("│  코드: %-4s %-4s %-4s        │\n",
               sw_code(m->sw_sequence[0]), sw_code(m->sw_sequence[1]), sw_code(m->sw_sequence[2]));
        printf("│  LED가 가리키는 버튼을 입력 │\n");
        led_write(ctx->fd_led, LED_ALL_OFF);
    } else {
        printf("│  [MIXED] 2단계 해제          │\n");
        printf("│  1. 선로:  "); print_wire_pattern(reverse_bits8(m->dip_target)); printf("  │\n");
        printf("│  2. 승인코드: %-4s           │\n", sw_code(m->confirm_button));
        led_write(ctx->fd_led, m->dip_target);
    }
    printf("└─────────────────────────────┘\n");
}

static int mission_fail(GameCtx *ctx)
{
    int j;
    printf("\n\n  [!!] 틀렸습니다! -5초 페널티\n");
    for (j = 0; j < 4; j++) {
        led_write(ctx->fd_led, LED_ALL_ON);
        usleep(100000);
        led_write(ctx->fd_led, LED_ALL_OFF);
        usleep(100000);
    }
    ctx->time_left -= 5;
    if (ctx->time_left < 0) ctx->time_left = 0;
    fnd_write_seconds(ctx->fd_fnd, ctx->time_left);
    mission_last_show = -1;
    return 0;
}

static int mission_success(GameCtx *ctx, int idx)
{
    printf("\n\n  [OK] 미션 %d 해제 성공!\n", idx + 1);
    led_write(ctx->fd_led, LED_ALL_ON);
    usleep(350000);
    led_write(ctx->fd_led, LED_ALL_OFF);
    mission_last_show = -1;
    prev_push_bits = 0;
    return 1;
}

static int mission_step(GameCtx *ctx, int idx)
{
    MissionSpec *m = &mission_specs[idx];
    unsigned char dip;
    unsigned short fresh;

    if (mission_last_show != idx) {
        mission_show(ctx, idx);
        mission_last_show = idx;
    }

    if (m->kind == MISSION_DIP_CODE) {
        dip = dip_read(ctx->fd_dip);
        printf("\r  현재 DIP:  "); print_wire_pattern(reverse_bits8(dip));
        printf(dip == m->dip_target ? "  일치. 노란 버튼으로 제출  " : "  불일치                  ");
        fflush(stdout);
        if (!yellow_button_pressed(ctx)) return 0;
        return (dip == m->dip_target) ? mission_success(ctx, idx) : mission_fail(ctx);
    }

    fresh = push_new_press(ctx->fd_push);
    if (m->kind == MISSION_SW_SEQUENCE) {
        led_write(ctx->fd_led, led_for_push(m->sw_sequence[m->sequence_pos]));
        printf("\r  진행: %d / %d   다음 코드: %-4s        ",
               m->sequence_pos, m->sequence_len, sw_code(m->sw_sequence[m->sequence_pos]));
        fflush(stdout);
        if (!fresh) return 0;
        if (fresh == m->sw_sequence[m->sequence_pos]) {
            m->sequence_pos++;
            if (m->sequence_pos >= m->sequence_len)
                return mission_success(ctx, idx);
            return 0;
        }
        m->sequence_pos = 0;
        return mission_fail(ctx);
    }

    dip = dip_read(ctx->fd_dip);
    led_write(ctx->fd_led, m->dip_target | led_for_push(m->confirm_button));
    printf("\r  선로: "); print_wire_pattern(reverse_bits8(dip));
    printf("  / 승인 코드: %-4s        ", sw_code(m->confirm_button));
    fflush(stdout);
    if (!fresh) return 0;
    if (dip == m->dip_target && fresh == m->confirm_button)
        return mission_success(ctx, idx);
    return mission_fail(ctx);
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
    ctx->fd_interrupt = interrupt_open();

    if (ctx->fd_led<0 || ctx->fd_fnd<0 || ctx->fd_dot<0 ||
        ctx->fd_dip<0 || ctx->fd_push<0 || ctx->fd_interrupt<0)
        return -1;

    ctx->state      = STATE_WAITING;
    ctx->time_left  = GAME_TIME_SEC;
    ctx->fire_frame = 0;
    ts_now(&ctx->last_frame);
    ts_now(&ctx->last_tick);
    last_interrupt_count = interrupt_read_count(ctx->fd_interrupt);

    led_write(ctx->fd_led, LED_ALL_OFF);
    fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
    dot_write_pattern(ctx->fd_dot, DOT_BOMB_FIRE[0]);

    print_banner();
    printf("  노란 인터럽트 버튼을 눌러 게임을 시작하세요.\n\n");
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
    interrupt_close(ctx->fd_interrupt);
}

/* ── game_update (메인 루프에서 매 10ms 호출) ─────────────── */

void game_update(GameCtx *ctx)
{
    switch (ctx->state) {

    /* ── WAITING ─────────────────────────────────────── */
    case STATE_WAITING:
        anim_bomb_fire(ctx);
        fnd_write_seconds(ctx->fd_fnd, GAME_TIME_SEC);
        yellow_diag_print(ctx);

        if (yellow_button_pressed(ctx)) {

            srand((unsigned int)time(NULL));
            seed_missions();
            prev_push_bits = 0;
            mission_last_show = -1;

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
            fnd_write_seconds(ctx->fd_fnd, ctx->time_left);
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
        printf("  노란 인터럽트 버튼을 눌러 게임을 시작하세요.\n\n");
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
        printf("  노란 인터럽트 버튼을 눌러 게임을 시작하세요.\n\n");
        ts_now(&ctx->last_frame);
        break;
    }
}
