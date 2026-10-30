# The program that shows current CAPSLOCK and LAYOUT state in a clickthrough overlay

## Configuration

### Style

Style can be configured via env variables:

- "CAPSLOCK_COLOR", default: "#FF5555"
- "CAPSLOCK_FONT", default: "Bold 30"
- "LAYOUT_COLOR", default: "#FF5555"
- "LAYOUT_FONT", default: "Bold 30"

### Default layout

Since this app shows *not default layout* state, it is possible to set default state.
To do this - aqquire the needed state number from `niri msg keyboard-layouts`
and provide via `LAYOUT_DEFAULT=n` env variable, where `n` is your layout number.

## Shown states

### Layout

Currently layout display works only with niri wayland compositor.
It relies on `niri msg keyboard-layouts` output to get the intitial layout state.
For watching on layout changes it uses `niri msg event-stream` command.
Currently it shows only *not-first-state* layout.
It's assuming that the first state is **EN** layout,
so when user switches to any other layout - it shows crisscrossed ~~EN~~ text.

### Capslock

It uses *libevdev* and some `/dev/input/event*` black magic.
Your user must be in *input* group, or run it with sudo.
