# Gates

A visual logic gate simulator. Write circuits in a simple text format, watch signals propagate get evaluated in real-time.

[Video demo](https://youtu.be/DbEJXLR-70E)

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
- Orange = Analog Signal 

To find out more visit the [GLL_Documentation.md](docs/GLL_Documentation.md)

## Build

Releases are currently Windows only for other Operating systems please build yourself.

Requires SFML 3.x and CMake and the libmodbus library. Either use one of the provided build scripts, or build manually. If you do not want to build GLL from source, visit our [releases](https://github.com/siekwiee/SFML-GLL-Sim/releases) page.

## Run

```bash
./build/Debug/GLLSimulator <Path to your .txt/.gll file>
```

Or drag and drop a .txt/.gll file onto the executable.
