#ifndef CONFIG_H
#define CONFIG_H

/* Device file paths */
#define DEV_LED   "/dev/fpga_led"
#define DEV_FND   "/dev/fpga_fnd"
#define DEV_DOT   "/dev/fpga_dot"
#define DEV_DIP   "/dev/fpga_dip_switch"
#define DEV_PUSH  "/dev/fpga_push_switch"

/* Game settings */
#define GAME_TIME_SEC      60    /* total countdown seconds */
#define FIRE_FRAME_MS      300   /* ms per fire animation frame */
#define MISSION_COUNT      3     /* number of missions */

/* LED bitmasks (8 LEDs: bit7=LED1 leftmost, bit0=LED8 rightmost) */
#define LED_ALL_ON   0xFF
#define LED_ALL_OFF  0x00
#define LED_BLINK_MS 200

/*
 * Push-switch 비트 (~/try/push_driver.c 기준)
 *   bit0=SW8, bit1=SW9, ..., bit8=SW16
 *   → SW8(bit0) = 0x0001 을 Start/Confirm 버튼으로 사용
 *   → SW9(bit1) = 0x0002 도 Confirm 으로 인정
 */
#define BTN_START   0x01   /* SW8 — 게임 시작 / 미션 제출 */
#define BTN_ANY     0x1FF  /* 9개 버튼 중 아무거나 */

#endif /* CONFIG_H */
