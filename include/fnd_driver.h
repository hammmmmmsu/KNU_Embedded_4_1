#ifndef FND_DRIVER_H
#define FND_DRIVER_H

int  fnd_open(void);
void fnd_close(int fd);

/* Display a value 0-99 on the rightmost two digits */
void fnd_write_seconds(int fd, int seconds);

/* Display raw 4-digit BCD array */
void fnd_write_digits(int fd, unsigned char d0, unsigned char d1,
                      unsigned char d2, unsigned char d3);

#endif /* FND_DRIVER_H */
