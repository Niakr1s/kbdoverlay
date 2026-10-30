#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

typedef void (*layout_cb)(int state, void *arg);

static int get_current_layout(int *layout) {
  FILE *f = popen("niri msg keyboard-layouts", "r");
  int res = 0;

  errno = 0;
  while (fscanf(f, " * %d", layout) != 1) {
    if (feof(f)) {
      res = 1;
      goto bail;
    }
    fscanf(f, "%*s[^\n]");
    fscanf(f, "%*c");
  }
  // we found match
  res = 0;

bail:
  pclose(f);
  return res;
}

int watch_layout(layout_cb cb, void *arg) {
  FILE *f = popen("niri msg event-stream", "r");

  int initial_layout;
  if (get_current_layout(&initial_layout) == 0)
    cb(initial_layout, arg);

  while(true) {
    int layout;
    if (fscanf(f, " Keyboard layout switched: %d", &layout) == 1) {
      cb(layout, arg);
    } else {
      fscanf(f, "%*[^\n]");   // skip up to newline
      fscanf(f, "%*c");       // consume the newline
    }
  }
  fclose(f);
}
