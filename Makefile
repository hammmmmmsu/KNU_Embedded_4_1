CC = gcc
CFLAGS = -Wall -Iinclude

TARGET = bomb_game

SRCS = src/main.c src/game.c src/mission.c \
       drivers/led_driver.c drivers/fnd_driver.c \
       drivers/dot_driver.c drivers/dip_driver.c \
       drivers/interrupt_driver.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o
