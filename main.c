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

struct Worker_state {
  char *label;
  int state;
};

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
  run_overlay();
  return 0;
}

// you should change it according to your workers count
#define WORKERS_N 2

static void print_states(struct Worker_state *states) {
  for (size_t i = 0; i < WORKERS_N; ++i) {
    printf("%s: %d; ", states[i].label, states[i].state);
  }
  printf("\n");
  fflush(stdout);
}

static void update_overlay(struct Worker_state *states) {
  bool should_show = false;
  char buf[256]; // this may not be enough
  char *ptr = buf;
  for (size_t i = 0; i < WORKERS_N; ++i) {
    if (states[i].state != 0) {
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

  struct Worker_state states[WORKERS_N] = {
      {.label = "<span foreground=\"#FF5555\" font=\"Bold 30\">⇧</span>", .state = 0},
      {.label = "<span foreground=\"#FF5555\" font=\"Bold 30\" strikethrough=\"true\">EN</span>", .state = 0},
  };

  thrd_t capslock_worker_thrd;
  thrd_create(&capslock_worker_thrd, capslock_worker, &states[0]);

  thrd_t layout_worker_thrd;
  thrd_create(&layout_worker_thrd, layout_worker, &states[1]);

  thrd_t state_reader_thrd;
  thrd_create(&state_reader_thrd, state_reader, &states);

  thrd_t ui_thrd;
  thrd_create(&ui_thrd, run_ui, NULL);

  thrd_join(layout_worker_thrd, NULL);

  mtx_destroy(&mtx_state);
  cnd_destroy(&cnd_state_changed);
}
