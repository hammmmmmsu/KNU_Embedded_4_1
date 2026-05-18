#ifndef DIP_DRIVER_H
#define DIP_DRIVER_H

int  dip_open(void);
void dip_close(int fd);

/* Read current DIP switch state (8 bits, 1=ON) */
unsigned char dip_read(int fd);

#endif /* DIP_DRIVER_H */
