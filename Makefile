# Simple Makefile for kbdoverlay
CC      := cc
PKG     := libevdev gtk4 gtk4-layer-shell-0
PKG_CFLAGS := $(shell pkg-config --cflags $(PKG))
PKG_LDLIBS := $(shell pkg-config --libs $(PKG))
CFLAGS  := -Wall -Wextra -O2  $(PKG_CFLAGS)
LDLIBS  := $(PKG_LDLIBS)
TARGET  := kbdoverlay
SRC     := main.c layout_watcher.c capslock_watcher.c ui.c
OBJ     := $(SRC:.c=.o)
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin

.PHONY: all clean run install uninstall

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -d -m 0755 $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o
