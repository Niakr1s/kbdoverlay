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
  char *foreground_color; // "#FF5555"
  char *font; // "Bold 30", "30"
  bool strikethrough;
};

static void label_with_style(struct Label_Style style, const char *label, char *buf, size_t sz) {
  snprintf(buf, sz, "<span foreground=\"%s\" font=\"%s\" strikethrough=\"%s\">%s</span>",
           style.foreground_color, style.font, style.strikethrough ? "true" : "false", label);
}

struct Worker_state {
  char *label;
  int state; // current state
  int off_state; // indicates which state should be considered as "off"
};

typedef int (*Worker_fn) (void *);

static void worker_state_set(struct Worker_state *w, int newstate) {
  mtx_lock(&mtx_state);
  w->state = newstate;
  cnd_signal(&cnd_state_changed);
  mtx_unlock(&mtx_state);
}

static void worker_cb_impl(int state, void *arg) {
  worker_state_set(arg, state);
}

// layout worker

static int layout_worker(void *arg) {
  printf("layout worker started\n");
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

static void print_states(struct Worker_state *states) {
  for (size_t i = 0; i < WORKERS_N; ++i) {
    printf("%s: %s; ", states[i].label, states[i].state == states[i].off_state ? "OFF" : "ON");
  }
  printf("\n");
  fflush(stdout);
}

static void update_overlay(struct Worker_state *states) {
  bool should_show = false;
  char buf[256]; // this may not be enough
  char *ptr = buf;
  for (size_t i = 0; i < WORKERS_N; ++i) {
    if (states[i].state != states[i].off_state) {
      should_show = true;
      int len = snprintf(ptr, 256 - (ptr - buf), "%s ", states[i].label);
      if (len != -1)
        ptr += len;
    }
  }
  overlay_set_text_cli(buf);
  if (should_show) 
    overlay_show_cli();
  else
    overlay_hide_cli();
}

static int state_reader(void *arg) {
  struct Worker_state *states = arg;
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
               .foreground_color = "#FF5555",
               .font = "Bold 30",
               .strikethrough = false,
  };
  char capslock_label[128];
  label_with_style(capslock_style, "⇧", capslock_label, 128);

  struct Label_Style layout_style = {
               .foreground_color = "#FF5555",
               .font = "Bold 30",
               .strikethrough = true,
  };
  char layout_label[128];
  label_with_style(layout_style, "EN", layout_label, 128);

  struct Worker_state states[WORKERS_N] = {
      {.label = capslock_label,
       .state = 0,
       .off_state = 0},
      {.label = layout_label,
       .state = 0,
       .off_state = 0},
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
