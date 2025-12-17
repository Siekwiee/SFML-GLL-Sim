# Gates

A visual logic gate simulator. Write circuits in a simple text format, watch signals propagate in real-time.

[Video demo](https://youtu.be/dmqBSP6_abg)

## Language

please refer to [GLL_Documentation.md](docs/GLL_Documentation.md)

## Controls

- **Space** - Play/Pause simulation
- **Period (.)** - Step once
- **+/-** - Speed up/slow down
- **Click** input widgets to toggle signals
- **Click** BTN widgets for momentary press
- **Ctrl+Click** BTN widgets to latch (hold state)

## Signals

- Green = HIGH (1)
- Red = LOW (0)

All IN signals get toggle widgets. BTN nodes are for momentary/latch behavior.

## Build

Requires SFML 3.x and CMake.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Run

```bash
./build/Debug/GLLSimulator samples/basic.txt
```
