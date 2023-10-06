CC = gcc
CFLAGS = -g -std=c99

all: schedule

schedule: schedule.c
	$(CC) $(CFLAGS) -o schedule schedule.c

clean:
	rm -f ./*.o schedule