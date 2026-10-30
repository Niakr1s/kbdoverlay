#pragma once

#include <stddef.h>

typedef void (*layout_cb)(int state, void *arg);

int watch_layout(layout_cb cb, void *arg);
int get_layout_name(int n, char *name, size_t sz);
