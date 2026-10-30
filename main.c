#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "capslock_watcher.h"
#include "layout_watcher.h"
#include "ui.h"

static mtx_t mtx_state;
static cnd_t cnd_state_changed;

struct Label_Style {
  char *color; // "#FF5555"
  char *font; // "Bold 30", "30"
  bool strikethrough;
};

/// Returns number of bytes printed
static int snprint_label_styled(char *buf, size_t sz, struct Label_Style style, const char *label) {
  return snprintf(buf, sz, "<span foreground=\"%s\" font=\"%s\" strikethrough=\"%s\">%s</span>",
           style.color, style.font, style.strikethrough ? "true" : "false", label);
}

struct KeyboardState {
  char label[8]; // 8 bytes will be enough
  struct Label_Style style;
  int state; // current state
  int off_state; // indicates which state should be considered as "off"
};

typedef int (*Worker_fn) (void *);

static void keyboard_state_set(struct KeyboardState *w, int newstate) {
  mtx_lock(&mtx_state);
  w->state = newstate;
  cnd_signal(&cnd_state_changed);
  mtx_unlock(&mtx_state);
}

static void worker_cb_impl(int state, void *arg) {
  keyboard_state_set(arg, state);
}

// layout worker

static int layout_worker(void *arg) {
  printf("layout worker started\n");
  struct KeyboardState *state = arg;

  // we need first 2 bytes + \0, so it's 3
  if (get_layout_name(state->off_state, state->label, 3) != 0) {
    fprintf(stderr, "couldn't aqquire layout label #%d, check layout number using 'niri msg keyboard-layouts'", state->off_state);
    exit(EXIT_FAILURE);
  } else {
    printf("aquired default layout label: %s\n", state->label);
  }
  watch_layout(worker_cb_impl, arg);
  return 0;
}

// capslock worker

static int capslock_worker(void *arg) {
  printf("capslock worker started\n");
  watch_caps_lock(worker_cb_impl, arg);
  return 0;
}

static int run_ui(void *arg) {
  (void) arg;
  run_overlay();
  return 0;
}

// you should change it according to your workers count
#define WORKERS_N 2

static void print_states(struct KeyboardState *states) {
  for (size_t i = 0; i < WORKERS_N; ++i) {
    printf("%s: %s; ", states[i].label, states[i].state == states[i].off_state ? "OFF" : "ON");
  }
  printf("\n");
  fflush(stdout);
}

static void update_overlay(struct KeyboardState *states) {
  bool should_show = false;
#define SZ 256
  char buf[SZ]; // this may not be enough
  char *ptr = buf;
  for (size_t i = 0; i < WORKERS_N; ++i) {
    if (states[i].state != states[i].off_state) {
      should_show = true;
      int len = snprint_label_styled(ptr, SZ - (ptr - buf), states[i].style, states[i].label);
      if (len != -1)
        ptr += len;
    }
  }
  overlay_set_text_cli(buf);
  if (should_show) 
    overlay_show_cli();
  else
    overlay_hide_cli();
#undef SZ
}

static int state_reader(void *arg) {
  struct KeyboardState *states = arg;
  for (;;) {
    mtx_lock(&mtx_state);
    cnd_wait(&cnd_state_changed, &mtx_state);
    print_states(states);
    update_overlay(states);

    mtx_unlock(&mtx_state);
  }
  return 0;
}

int main() {
  mtx_init(&mtx_state, mtx_plain);
  cnd_init(&cnd_state_changed);

  struct Label_Style capslock_style = {
               .color = "#FF5555",
               .font = "Bold 30",
               .strikethrough = false,
  };
  struct KeyboardState capslock_state = {.label = "⇧", .style = capslock_style, .off_state = 0};

  struct Label_Style layout_style = {
               .color = "#FF5555",
               .font = "Bold 30",
               .strikethrough = true,
  };
  // for layout it's label will be set inside worker
  struct KeyboardState layout_state = {.style = layout_style, .off_state = 0};

  struct KeyboardState states[WORKERS_N] = {
    capslock_state,
    layout_state,
  };

  Worker_fn workers[WORKERS_N] = {
    capslock_worker,
    layout_worker,
  };

  // make space for state_reader and ui threads
  thrd_t thrds[WORKERS_N + 2];

  for (size_t i = 0; i < WORKERS_N; ++i) {
    thrd_create(&thrds[i], workers[i], &states[i]);
  }

  thrd_create(&thrds[WORKERS_N + 0], state_reader, &states);
  thrd_create(&thrds[WORKERS_N + 1], run_ui, NULL);

  int res = 0;

  for (size_t i = 0; i < WORKERS_N+2; ++i) {
    thrd_join(thrds[i], &res);
    if (res)
      printf("error in thrd #%zu: %d", i, res);
  }


  mtx_destroy(&mtx_state);
  cnd_destroy(&cnd_state_changed);
}
