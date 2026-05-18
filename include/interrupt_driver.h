#ifndef INTERRUPT_DRIVER_H
#define INTERRUPT_DRIVER_H

int  push_open(void);
void push_close(int fd);

/* Read current button state. Returns bitmask (bit7=SW1). 0=no press. */
unsigned char push_read(int fd);

/* Non-blocking: returns 1 if btn_mask buttons are pressed, else 0 */
int push_pressed(int fd, unsigned char btn_mask);

#endif /* INTERRUPT_DRIVER_H */
