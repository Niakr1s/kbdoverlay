# Simple Makefile for kbdoverlay
CC      := cc
CFLAGS  := -Wall -Wextra -O2 -levdev
TARGET  := kbdoverlay
SRC     := main.c layout_watcher.c capslock_watcher.c

.PHONY: all clean run install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -m 0755 $(TARGET) /usr/local/bin

uninstall:
	rm -f /usr/local/bin/$(TARGET)

clean:
	rm -f $(TARGET)
