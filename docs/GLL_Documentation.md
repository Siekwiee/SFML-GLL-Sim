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

### **Gates**

Gates are used to descibe the logic operations in the circut. Each gate is declared with a type, a name, a parenthesized list of inputs, and a comma-separated list of outputs, coming after the `->`.

The currently supported gates are:

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

### Other nodes

BTN nodes are used to describe buttons that can be pressed or released. not done yet (WIP).

### **Syntax-Sugar**

#### multiple outputs

If a gate has multiple outputs, they can be declared in a comma-separated list after the `->`.

example:

```
AND gate1(a, b) -> c, d
```
