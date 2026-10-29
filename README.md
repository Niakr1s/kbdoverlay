# The program that shows current CAPSLOCK and LAYOUT state in a clickthrough overlay

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

## TODO

- [ ] Take keyboard layout name from `niri msg keyboard-layouts`
- [ ] Get format options from command line arguments
- [ ] Get capslock symbol from command line arguments
