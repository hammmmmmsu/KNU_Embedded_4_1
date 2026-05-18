#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    int fd;
    char buf[8];

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd >= 0) {
        write(fd, "27", 2);
        close(fd);
    }

    fd = open("/sys/class/gpio/gpio27/direction", O_WRONLY);
    if (fd >= 0) {
        write(fd, "in", 2);
        close(fd);
    }

    while (1) {
        fd = open("/sys/class/gpio/gpio27/value", O_RDONLY);
        if (fd < 0) {
            perror("open value");
            return 1;
        }

        memset(buf, 0, sizeof(buf));
        read(fd, buf, sizeof(buf) - 1);
        close(fd);

        printf("\rGPIO27=%c", buf[0]);
        fflush(stdout);
        usleep(100000);
    }

    return 0;
}
