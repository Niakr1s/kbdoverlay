# Simple Makefile for kbdoverlay
CC      := cc
CFLAGS  := -Wall -Wextra -O2 -levdev `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0`
TARGET  := kbdoverlay
SRC     := main.c layout_watcher.c capslock_watcher.c ui.c

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
