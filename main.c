#include <assert.h>
#include <stdio.h>
#include <threads.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "layout_watcher.h"
#include "capslock_watcher.h"

static mtx_t mtx_state;
static cnd_t cnd_state_changed;

struct Worker_state {
  char *label;
  int state;
};

void worker_state_set(struct Worker_state *w, int newstate) {
  mtx_lock(&mtx_state);
  w->state = newstate;
  cnd_signal(&cnd_state_changed);
  mtx_unlock(&mtx_state);
}

static void worker_cb_impl(int state, void *arg) {
  worker_state_set(arg, state);
}

// layout worker

int layout_worker(void *arg) {
  printf("layout worker started\n");
  watch_layout(worker_cb_impl, arg);
  return 0;
}

// capslock worker

int capslock_worker(void *arg) {
  printf("capslock worker started\n");
  watch_caps_lock(worker_cb_impl, arg);
  return 0;
}

// you should change it according to your workers count
#define WORKERS_N 2

int state_reader(void *arg) {
  struct Worker_state *states = arg;
  for (;;) {
    mtx_lock(&mtx_state);
    cnd_wait(&cnd_state_changed, &mtx_state);
    printf("States:\n");
    for (size_t i = 0; i < WORKERS_N; ++i) {
      printf("%s: %d\n", states[i].label, states[i].state);
    }
    mtx_unlock(&mtx_state);
  }
  return 0;
}

int main() {
  mtx_init(&mtx_state, mtx_plain);
  cnd_init(&cnd_state_changed);

  struct Worker_state states[WORKERS_N] = {
    {.label = "layout", .state = 0},
    {.label = "capslock", .state = 0},
  };

  thrd_t layout_worker_thrd;
  thrd_create(&layout_worker_thrd, layout_worker, &states[0]);

  thrd_t capslock_worker_thrd;
  thrd_create(&capslock_worker_thrd, capslock_worker, &states[1]);

  thrd_t state_reader_thrd;
  thrd_create(&state_reader_thrd, state_reader, &states);

  thrd_join(layout_worker_thrd, NULL);

  mtx_destroy(&mtx_state);
  cnd_destroy(&cnd_state_changed);
}
