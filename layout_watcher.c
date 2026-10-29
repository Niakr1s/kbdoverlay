#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

typedef void (*layout_cb)(int state, void *arg);

static int get_current_layout(int *layout) {
  FILE *f = popen("niri msg keyboard-layouts", "r");
  int res = 0;

  char buf[32];
  errno = 0;
  while (fgets(buf, 32, f) != NULL) {
    if (fscanf(f, " * %d", layout) == 1) {
      goto bail;
    }
  }
  // we didn't found match
  res = 1;
  if (errno != 0) {
    perror(__func__);
  }

bail:
  pclose(f);
  return res;
}

int watch_layout(layout_cb cb, void *arg) {
  FILE *f = popen("niri msg event-stream", "r");

  int initial_layout;
  if (get_current_layout(&initial_layout) == 0)
    cb(initial_layout, arg);

  char buf[256];

  while(true) {
    errno = 0;
    if (fgets(buf, 256, f) == NULL && errno != 0) {
      perror(__func__);
      return 1;
    }

    int layout;
    if (sscanf(buf, "Keyboard layout switched: %d", &layout) == 1) {
      cb(layout, arg);
    }
  }
  fclose(f);
}
