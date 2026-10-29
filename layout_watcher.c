#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

typedef void (*layout_cb)(int state, void *arg);

int watch_layout(layout_cb cb, void *arg) {
  FILE *f = popen("niri msg event-stream", "r");

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
