#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <threads.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

int niri_layout_worker(void *arg) {
  struct Worker_state *w = arg;
  printf("niri layout worker started\n");

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
      worker_state_set(w, layout);
    }
  }
  return 0;
}

int state_reader(void *arg) {
  struct Worker_state *states = arg;
  for (;;) {
    mtx_lock(&mtx_state);
    cnd_wait(&cnd_state_changed, &mtx_state);
    printf("States:\n");
    for (size_t i = 0; i < 1; ++i) {
      printf("%s: %d\n", states->label, states->state);
    }
    mtx_unlock(&mtx_state);
  }
  return 0;
}

int main() {
  mtx_init(&mtx_state, mtx_plain);
  cnd_init(&cnd_state_changed);

  struct Worker_state states[] = {
    {.label = "layout", .state = 0},
  };

  thrd_t niri_layout_worker_thrd;
  thrd_create(&niri_layout_worker_thrd, niri_layout_worker, &states[0]);

  thrd_t state_reader_thrd;
  thrd_create(&state_reader_thrd, state_reader, &states);

  thrd_join(niri_layout_worker_thrd, NULL);

  mtx_destroy(&mtx_state);
  cnd_destroy(&cnd_state_changed);
}
