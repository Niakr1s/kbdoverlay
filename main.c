#include <stdio.h>
#include <threads.h>
#include <stdlib.h>

#define THREADS_N 2

static const char *cmds[THREADS_N] = {
    "while true; do echo 'cmd1'; sleep 1; done",
    "while true; do echo 'cmd2'; sleep 1; done",
};

int run(void *arg) {
  size_t n = *(size_t *)arg;
  free(arg);
  printf("thread #%zu started, it's cmd is: %s\n", n, cmds[n]);
  return 0;
}

int main() {
  thrd_t threads[THREADS_N];

  for (size_t i = 0; i < THREADS_N; ++i) {
    size_t *n = malloc(sizeof(*n));
    *n = i;
    thrd_create(&threads[i], run, n);
  }

  for (size_t i = 0; i < THREADS_N; ++i) {
    thrd_join(threads[i], NULL);
  }
}
