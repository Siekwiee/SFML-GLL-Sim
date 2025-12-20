# GLL (`Gate logic Language`) Reference document

> Disclaimer: This is a work of art, and hobbyism. I'm not following any norms, unless they are actually making sense. This is not ment for any kind of production use. Be ready to see behavior that may not be what you expect.

## Overview

`GLL` is a simple language for describing logic circuits. It is designed to be easy to read and write, and to be used for prototyping and educational purposes.

## Syntax

### **Comments**

Comments start with a `#` and continue to the end of the line. Multi-line comments are yet to be implemented.

```
# This is a comment
```

### **Input and output signals**

Input and output signals are declared using the `IN` and `OUT` keywords, respectively. Each signal is declared with a symbol-name, in a comma-separated list of input and output names. Read more about multiple outputs [here](#multiple-outputs).
Every Input or Output signal must be declared here accept if its used temporarily once in the circuit.

```
IN a, b, c
OUT x, y, z
```

### **Nodes**

Nodes are used to descibe the logic operations in the circut. Each gate is declared with a type, a name, a parenthesized list of inputs, and a comma-separated list of outputs, coming after the `->`.

The currently supported nodes are:

#### `AND`

The `AND` gate is used to compute the logical `AND` of its inputs. The output is `true` if and only if all inputs are `true`.

```
AND gate1(a, b) -> c
```

**Truth table**:
| A | B | C |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 1 |

#### `OR`

The `OR` gate is used to compute the logical `OR` of its inputs. The output is `true` if at least one input is `true`. If no inputs are `true`, the output is `false`.

```
OR gate2(a, b) -> c
```

**Truth table**:
| A | B | C |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 1 |
| 1 | 0 | 1 |
| 1 | 1 | 1 |

#### `XOR`

The `XOR` gate is used to compute the logical `XOR` of its inputs. The output is `true` if the inputs are different, and `false` if the inputs are the same.

```
XOR gate2(a, b) -> c
```

**Truth table**:
| A | B | C |
| --- | --- | --- |
| 0 | 0 | 0 |
| 0 | 1 | 1 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

#### `NOT`

The `NOT` gate is used to compute the logical `NOT` of its input. The output is `true` if the input is `false`, and `false` if the input is `true`. Basically, it inverts the input signal. A `NOT` gate should be defined inline as in the example below.

```
AND gate3(a, NOT(b)) -> c
```

This makes the circuit "code" more dense and easier/natural to read.

**Truth table**:
| B | X |
| --- | --- |
| 0 | 1 |
| 1 | 0 |

#### `SR`

The `SR` gate is used to compute a `set/reset` position of its inputs. The reset priority in the `SR` variant in `GLL` differs from most known Norms, where for what ever reason the reset priority is contridictant to the naming, in `GLL` that is not the case. The input on `S` overrides the reset on `R`. Meaning if both inputs are `true`, the output is `true`.

- Inputs: `S` (set), `R` (reset)

- Outputs: `Q`

example:

```
SR setBlock(a, b) -> c
```

**Truth table**:
| S | R | Q |
| --- | --- | --- |
| 0 | 0 | Ø |
| 0 | 1 | 0 |
| 1 | 0 | 1 |
| 1 | 1 | 1 |

#### `RS`

The `RS` gate is used to compute a `set/reset` position of its inputs. The reset priority in the `RS` variant in `GLL` differs from most known Norms, where for what ever reason the reset priority is in contradiction to the naming, in `GLL` that is not the case. The input on `S` is overriden by the reset on `R`. Meaning if both inputs are `true`, the output is `false`, due to the dominant `R` input resetting.

- Inputs: `S` (set), `R` (reset)

- Outputs: `Q`

example:

```
RS setBlock(a, b) -> c
```

**Truth table**:
| S | R | Q |
| --- | --- | --- |
| 0 | 0 | Ø |
| 0 | 1 | 0 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

#### `TON`

The `TON` gate is a timer on delay. When the input goes HIGH, the timer starts counting. The output goes HIGH only after the input has been HIGH for the preset time (PT) duration. If the input goes LOW before the preset time elapses, the timer resets and the output remains LOW.

- Inputs: `IN` (input signal)

- Outputs: `Q`

- Preset Time (PT): Configured in the UI sidebar for each TON gate, or **hardcoded** in the GLL file as the first argument. Click the timer widget to edit. Supports formats like `500ms` (milliseconds), `10s` (seconds), `2m` (minutes), `1h` (hours).

**Hardcoded Example**:

```
# The first argument is a string or number with unit (fixed time)
TON delay_on("400ms", start) -> delayed_output
```

If a hardcoded time is provided, it overrides defaults and cannot be changed in the UI.

example:

```
TON delay_on(start) -> delayed_output
```

**Behavior**:

- Input goes HIGH → Timer starts counting
- Input stays HIGH for PT duration → Output goes HIGH
- Input goes LOW before PT duration → Timer resets, output stays LOW
- Input goes LOW after output is HIGH → Output goes LOW immediately

#### `TOF`

The `TOF` gate is a timer off delay. When the input goes LOW, the timer starts counting. The output stays HIGH for the preset time (PT) duration after the input goes LOW, then goes LOW. If the input goes HIGH again before the timer expires, the timer resets and the output stays HIGH.

- Inputs: `IN` (input signal)

- Outputs: `Q`

- Preset Time (PT): Configured in the UI sidebar for each TOF gate, or **hardcoded** in the GLL file as the first argument. Click the timer widget to edit. Supports formats like `500ms` (milliseconds), `10s` (seconds), `2m` (minutes), `1h` (hours).

**Hardcoded Example**:

```
# Fixed 2 second off-delay
TOF delay_off("2s", stop) -> delayed_output
```

If a hardcoded time is provided, it overrides defaults and cannot be changed in the UI.

example:

```
TOF delay_off(stop) -> delayed_output
```

**Behavior**:

- Input goes HIGH → Output goes HIGH immediately
- Input goes LOW → Timer starts counting, output stays HIGH
- Timer expires (PT duration) → Output goes LOW
- Input goes HIGH again before timer expires → Timer resets, output stays HIGH

#### `CTU` (Work In Progress)

The `CTU` gate is a count-up timer.

- Inputs: `CU` (count up), `R` (reset), `PV` (preset value)
- Outputs: `Q` (done), `CV` (current value)

_Note: Implementation is currently incomplete._

#### `BTN` (Work In Progress)

`BTN` nodes represent physical buttons in the simulation. They can be momentary or latched.

- Outputs: `Q` (high when pressed/latched)

example:

```
BTN myButton() -> is_pressed
```

### Integration

#### **Modbus TCP**

GLL supports Modbus TCP as a client. It can synchronize its internal signals with a remote Modbus server (like from Factory I/O).

- **Inputs**: Signals named `INPUT_0`, `INPUT_1`, ..., `INPUT_N` are automatically mapped to Modbus Discrete Inputs or Input Registers.
- **Outputs**: Signals named `OUTPUT_0`, `OUTPUT_1`, ..., `OUTPUT_N` are automatically mapped to Modbus Coils or Holding Registers.

Mapping configuration (IP, Port, Slave ID, and Bit Counts) can be adjusted in the **Settings** menu. These settings are saved to `modbus_config`.

### **Simulation Features**

#### **Hot Reloading**

The simulator monitors the loaded `.gll` or `.txt` file for changes. When you save the file in your external editor, Gates will automatically re-parse the logic and refresh the simulation state without needing to restart.

#### **Execution Modes**

- **Play/Pause**: Use the **Space** key or the Play button in the sidebar to start/stop the simulation.
- **Step Once**: Use the **Period (.)** key or the Step button to advance the simulation by one node evaluation.
- **Repeat/Once**: Toggle between continuous execution and single-cycle execution in the sidebar.

#### **Speed Control**

Use the `+` and `-` keys or the slider in the sidebar to adjust the simulation speed (Evaluation frequency). At lower speeds, you can see the signal propagation highlighted line-by-line.

### **Controls**

- **Space** - Play/Pause simulation
- **Period (.)** - Step once evaluation
- **+/-** - Speed up/slow down simulation frequency
- **Click** input widgets to toggle signals
- **Click** BTN widgets for momentary press
- **Ctrl+Click** BTN widgets to latch (hold state)
- **Click** Timer widgets to edit preset time (if not hardcoded)

### **Execution Model**

Gates follows a basic PLC-style single-pass scan:

1.  **Read Inputs**: External signals (Modbus, UI toggles) are captured.
2.  **Sequential Evaluation**: Nodes are evaluated one by one in the order they appear in the file.
3.  **Forward References**: If a node uses an input from a node defined _later_ in the file, it will use the signal value from the _previous_ scan cycle. If it uses a signal from an _earlier_ node, it uses the fresh value from the current cycle.
4.  **Write Outputs**: Results are committed to output signals and Modbus coils.

This simple model ensures predictable behavior even with complex logic cycles.

### **Syntax-Sugar**

#### multiple outputs

If a gate has multiple outputs, they can be declared in a comma-separated list after the `->`.

example:

```
AND gate1(a, b) -> c, d
```

#### Aliasing

Aliases can be used to give a signal a more readable name. This is useful for readability and for debugging.

example:

```
IN INPUT_0(atEntry), INPUT_1(atLoad), INPUT_2(atLeft), INPUT_3(atRight)
OUT OUTPUT_0(entryConv), OUTPUT_1(loadConv), OUTPUT_2(forksL), OUTPUT_3(forksR)

AND Step1(atEntry, atLoad) -> entryConv
OR Step2(atLeft, atRight) -> forksL, forksR, loadConv
```
