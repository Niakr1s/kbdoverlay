#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/input.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <errno.h>

/* Callback: invoked whenever Caps Lock LED state changes. */
typedef void (*caps_cb)(int state, void *user);

/*
 * Open the first readable /dev/input/event* device that advertises a
 * Caps Lock LED. Returns fd, or -1 if none found.
 */
static int find_capslock_device(void)
{
    DIR *d = opendir("/dev/input");
    if (!d) {
        perror("opendir /dev/input");
        return -1;
    }

    int fd = -1;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "event", 5) != 0)
            continue;

        char path[300];
        snprintf(path, sizeof(path), "/dev/input/%s", ent->d_name);

        int candidate = open(path, O_RDONLY | O_NONBLOCK);
        if (candidate < 0)
            continue;  /* permission denied, busy, etc. */

        struct libevdev *dev = NULL;
        if (libevdev_new_from_fd(candidate, &dev) < 0) {
            close(candidate);
            continue;
        }

        int has_capsl = libevdev_has_event_code(dev, EV_LED, LED_CAPSL);
        libevdev_free(dev);

        if (has_capsl) {
            /* Clear O_NONBLOCK so read() blocks. */
            int flags = fcntl(candidate, F_GETFL);
            if (flags >= 0)
                fcntl(candidate, F_SETFL, flags & ~O_NONBLOCK);
            fd = candidate;
            break;
        }
        close(candidate);
    }
    closedir(d);
    return fd;
}

/*
 * Block on the fd, calling cb(state, user) on each EV_LED/LED_CAPSL event.
 * Returns 0 on clean EOF, -1 on error.
 */
int watch_caps_lock(caps_cb cb, void *user)
{
    int fd = find_capslock_device();
    if (fd < 0) {
        fprintf(stderr, "caps: no keyboard with Caps Lock LED found\n");
        return -1;
    }

    struct input_event ev;
    for (;;) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n < 0) {
            if (errno == EINTR)
                continue;
            perror("read");
            close(fd);
            return -1;
        }
        if (n == 0) {            /* device removed */
            close(fd);
            return 0;
        }
        if ((size_t)n < sizeof(ev))  /* partial read — shouldn't happen */
            continue;

        if (ev.type == EV_LED && ev.code == LED_CAPSL)
            cb(ev.value ? 1 : 0, user);
    }
}

static void on_caps(int state, void *user) {
    printf("Caps Lock: %s\n", state ? "ON" : "OFF");
    fflush(stdout);
}

int main(void) {
    return watch_caps_lock(on_caps, NULL) < 0 ? 1 : 0;
}
