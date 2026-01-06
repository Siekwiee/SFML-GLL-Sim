# Changelog

## All notable changes to **Gates** (the SFML **GLL** logic simulator) will be documented in this file.

## 0.0.6 - 2026-01-06

- Added AIN (Analog Input) and AOUT (Analog Output) signal declarations for 8-bit hex values (0x00-0xFF)
- Added hex literal support in comparators ("0xFF", "0x80") and decimal literals ("128")
- Counter CV outputs, analog signals, and literals are all compatible for comparisons
- Added analog input widgets with editable values and HEX/DEC toggle switch
- Added analog output widgets (read-only display)
- Added Modbus TCP support for analog I/O (AINPUT_N/AOUTPUT_N mapped to registers)
- Removed ESC key closing the application instead ESC jumps out of the current selected text input and reverts changes made.
- Updated [GLL_Documentation.md](GLL_Documentation.md#Syntax) with AIN/AOUT syntax and added samples/tests/analog_comparators.gll



## 0.0.5 - 2025-12-29

- add support for not full number time values to be delared with .5s, .25s, etc
- Implemented PS (Positive Signal/Rising Edge) node for detecting rising edges
- Added inline PS() syntax support (similar to NOT()) for use in gate arguments
- Implemented NS (Negative Signal/Falling Edge) node for detecting falling edges
- Added inline NS() syntax support (similar to NOT() and PS()) for use in gate arguments
- Added CV (Counter Value) output extraction for CTU/CTD counters via second output variable
- Implemented LT (Less Than), GT (Greater Than), and EQ (Equal) comparator gates
- Comparators work with CV outputs to compare counter values

## 0.0.4 - 2025-12-20

- Dynamic IO bit counts for Modbus configarable in settings, and auto-saved to disk
- Added support for TON/TOF nodes with hardcoded preset times in Gll
- Uniformised time parsing in UI and Gll
- Added millisecond support
- Implemented CTU (Counter Up) and CTD (Counter Down) nodes
- Added support for hardcoded PV/CV arguments in CTU/CTD
- Added UI widgets for viewing and editing counter PV/CV values
- Fixed castSignalToBool\_ returning true for all valid signal indices regardless of value
- Updated GLL_Documentation.md with CTU/CTD details
- Added Scrolling behavior to the code part

## 0.0.3 - 2025-12-20

- Reversed to having a basic PLC single-pass scan from top to bottom without any fancy ready checks nor sorting.
- Added forward reference detection
- Added a Sample for a switch with a SR that has one input for set and reset, containing such forward pass.
- Added input buffering till the end of a cycle (currently only counts for basic signal inputs)
- Added support for higher frequencies (UNTESTED)
- Added ModBus Client Support
- Added FactoryIO A-To-B GLL example
- Added alias support for inputs and outputs see [Aliasing](GLL_Documentation.md#aliasing)
- Hardened font loading and added fallback
- Capped max sim speed at 2000 Hz for now
- Added XOR gate support see [XOR-Gate](GLL_Documentation.md#xor)

## 0.0.2 - 2025-12-19

Added **TON** and **TOF** timer gates with interactive preset time (PT) configuration:

- **TON** (Timer On Delay): Output goes HIGH after input has been HIGH for the preset time duration
- **TOF** (Timer Off Delay): Output stays HIGH for the preset time duration after input goes LOW
- Timer widgets appear in the sidebar for each TON/TOF gate
- Click timer widgets to edit preset time (PT) - supports formats like `500ms`, `10s`, `2m`, `1h` (milliseconds, seconds, minutes, hours)
- Timer widgets highlight yellowish when active (timer is ticking), grayish when inactive
- Preset time can be configured at runtime through the UI sidebar

- **hot reloading**
- **run loop toggle button**

## 0.0.1 - 2025-12-18

Baseline version matching the currenty released program behavior, with the following feature differences:

- No **hot reloading**: editing the loaded `.txt/.gll` file did not automatically re-parse and refresh the simulator/UI.
- No **run loop toggle button**: there was no `REPEAT/ONCE` control; when running, the simulation always looped continuously (it could not auto-stop after a single run cycle).
