#pragma once

typedef void (*layout_cb)(int state, void *arg);

int watch_layout(layout_cb cb, void *arg);
