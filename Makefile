CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude -Iassets

TARGET = bomb_game

# User-space application sources only.
# Kernel module sources in drivers/ are compiled separately with kbuild.
SRCS = src/main.c \
       src/game.c \
       src/state.c \
       src/timer.c \
       src/mission.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.o

# ── Kernel module build (run on the Raspberry Pi) ──────────
# Usage: make modules
# Requires kernel headers: sudo apt install raspberrypi-kernel-headers
KDIR ?= /lib/modules/$(shell uname -r)/build

modules:
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers modules

modules_clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/drivers clean

.PHONY: all clean modules modules_clean
