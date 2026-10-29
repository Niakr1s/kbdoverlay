#pragma once

typedef void (*caps_cb)(int state, void *arg);

int watch_caps_lock(caps_cb cb, void *user);
