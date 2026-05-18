#ifndef DOT_DRIVER_H
#define DOT_DRIVER_H

#define DOT_ROW_COUNT 10

int  dot_open(void);
void dot_close(int fd);

/* Write a raw 10-byte row pattern to the DOT matrix */
void dot_write_pattern(int fd, const unsigned char *pattern);

/* Clear all dots */
void dot_clear(int fd);

#endif /* DOT_DRIVER_H */
