CC=gcc
CFLAGS=-Wall -Wextra -O2 -Iinclude
TARGET=backup_sync

SRC=src/main.c src/daemon.c src/watcher.c src/backup.c src/logger.c src/utils.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run:
	./$(TARGET) demo_source demo_backup --foreground

stop:
	./$(TARGET) demo_source demo_backup --stop
