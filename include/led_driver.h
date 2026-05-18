#ifndef LED_DRIVER_H
#define LED_DRIVER_H

int  led_open(void);
void led_close(int fd);
void led_write(int fd, unsigned char mask);

#endif /* LED_DRIVER_H */
