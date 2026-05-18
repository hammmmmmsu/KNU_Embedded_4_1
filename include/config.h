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

/* Push-switch bitmasks (active-high after driver inversion) */
#define BTN_START   0x80   /* SW1 - game start / mission confirm */
#define BTN_ANY     0xFF

#endif /* CONFIG_H */
