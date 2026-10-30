#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

typedef void (*layout_cb)(int state, void *arg);

/// Gets layout name for N th layout
/// The name will be written to NAME, with strlen < SZ and the last char set to \0
/// Return: 0 if OK, else -1
int get_layout_name(int n, char *name, size_t sz) {
  FILE *f = popen("niri msg keyboard-layouts", "r");


  // Example output:
  //
  //Keyboard layouts:
  // * 0 English (US)
  //   1 Russian

  int res = 0;
  errno = 0;

  int scan_result;
  int layout_n;
  char buf[sz];
  while ((scan_result = fscanf(f, "%*c%*c%*c%d %s", &layout_n, buf)) != EOF) {
    if (scan_result == 2 && layout_n == n) {
      res = 0;
      strncpy(name, buf, sz);
      name[sz-1] = '\0';
      goto bail;
    }
    fscanf(f, "%*s[^\n]");
    fscanf(f, "%*c");
  }
  if (feof(f)) {
    res = -1;
    goto bail;
  }
  // we found match
  res = 0;

bail:
  pclose(f);
  return res;
}

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
