#ifndef CONFIG_H
#define CONFIG_H

/* Device file paths */
#define DEV_LED   "/dev/fpga_led"
#define DEV_FND   "/dev/fpga_fnd"
#define DEV_DOT   "/dev/fpga_dot"
#define DEV_DIP   "/dev/fpga_dip_switch"
/* DEV_PUSH 제거 — 브레드보드 버튼은 GPIO sysfs로 직접 읽음 */

/* Game settings */
#define GAME_TIME_SEC      60    /* total countdown seconds */
#define FIRE_FRAME_MS      300   /* ms per fire animation frame */
#define MISSION_COUNT      3     /* number of missions */

/* LED bitmasks (8 LEDs: bit7=LED1 leftmost, bit0=LED8 rightmost) */
#define LED_ALL_ON   0xFF
#define LED_ALL_OFF  0x00
#define LED_BLINK_MS 200

/*
 * 브레드보드 노란 버튼 → EXT_GP102 → Raspberry Pi BCM GPIO
 * 틀릴 경우: Pi에서 'raspi-gpio get' 실행 후 올바른 번호로 수정
 */
#define BTN_GPIO_NUM  27   /* EXT_GP102 = BCM 27 (틀리면 수정) */

#endif /* CONFIG_H */
