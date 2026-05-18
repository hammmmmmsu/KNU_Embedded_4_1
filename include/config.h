#ifndef CONFIG_H
#define CONFIG_H

/* Device file paths */
#define DEV_LED   "/dev/fpga_led"
#define DEV_FND   "/dev/fpga_fnd"
#define DEV_DOT   "/dev/fpga_dot"
#define DEV_DIP   "/dev/fpga_dip_switch"
#define DEV_PUSH  "/dev/fpga_push_switch"
#define DEV_INTERRUPT "/dev/fpga_interrupt"

/* Game settings */
#define GAME_TIME_SEC      60    /* total countdown seconds */
#define FIRE_FRAME_MS      300   /* ms per fire animation frame */
#define MISSION_COUNT      3     /* number of missions */

/* LED bitmasks (8 LEDs: bit7=LED1 leftmost, bit0=LED8 rightmost) */
#define LED_ALL_ON   0xFF
#define LED_ALL_OFF  0x00
#define LED_BLINK_MS 200

/* FPGA 보드 위 검은 푸시 스위치 (SW8~SW16)
 * bit0=SW8, bit1=SW9, ..., bit8=SW16
 * 게임 시작/제출은 브레드보드 노란 버튼(GPIO27 interrupt)을 사용한다.
 */

#endif /* CONFIG_H */
