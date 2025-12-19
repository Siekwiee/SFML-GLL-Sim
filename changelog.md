# Changelog

All notable changes to **Gates** (the SFML **GLL** logic simulator) will be documented in this file.

## 0.0.2 - 2025-12-19

Added **TON** and **TOF** timer gates with interactive preset time (PT) configuration:

- **TON** (Timer On Delay): Output goes HIGH after input has been HIGH for the preset time duration
- **TOF** (Timer Off Delay): Output stays HIGH for the preset time duration after input goes LOW
- Timer widgets appear in the sidebar for each TON/TOF gate
- Click timer widgets to edit preset time (PT) - supports formats like `10s`, `2m`, `1h` (seconds, minutes, hours)
- Timer widgets highlight yellowish when active (timer is ticking), grayish when inactive
- Preset time can be configured at runtime through the UI sidebar

- **hot reloading**
- **run loop toggle button**

## 0.0.1 - 2025-12-18

Baseline version matching the currenty released program behavior, with the following feature differences:

- No **hot reloading**: editing the loaded `.txt/.gll` file did not automatically re-parse and refresh the simulator/UI.
- No **run loop toggle button**: there was no `REPEAT/ONCE` control; when running, the simulation always looped continuously (it could not auto-stop after a single run cycle).
