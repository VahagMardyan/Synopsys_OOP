# VHG Language Documentation

## Table of Contents

- [Highlights](#highlights)
- [Project Structure](#project-structure)
- [Building the Compiler](#building-the-compiler)
  - [Requirements](#requirements)
  - [Runner files](#runner-files)
    - [Linux, MacOS or WSL](#linux-macos-or-wsl)
    - [Windows](#windows)
- [Usage](#usage)
  - [1. Run source directly](#1-run-source-directly-backwardcompatible)
  - [2. Compile to bytecode](#2-compile-to-bytecode)
  - [3. Run pre‑compiled bytecode](#3-run-precompiled-bytecode)
  - [4. Compile to object unit](#4-compile-to-object-unit)
  - [5. Link object units](#5-link-object-units)
- [Linker](#linker)
- [Language Syntax Overview](#-language-syntax-overview)
  - [Variables &amp; Scoping](#variables--scoping)
  - [Variable Declaration Rules](#variable-declaration-rules)
  - [It is desirable to know](#it-is-desirable-to-know)
  - [Data Types](#data-types)
  - [String Indexing](#string-indexing)
  - [Arrays](#arrays)
  - [Operators](#operators)
  - [Mathematical Functions](#mathematical-functions)
  - [Mathematical Constants](#mathematical-constants)
  - [Control Flow](#control-flow)
  - [Functions](#functions)
  - [Program entry (`main function`)](#program-entry-main)
  - [Switch/case](#switchcase)
  - [Built‑in I/O](#builtin-io)
  - [Number Syntax](#the-syntax-of-number-types)
  - [Import Preprocessing](#import-preprocessing)
- [Architecture Deep Dive](#-architecture-deep-dive)
  - [Lexer &amp; Tokenizer](#lexer--tokenizer)
  - [Parser](#parser)
  - [Symbol Table](#symbol-table)
  - [Compiler](#compiler)
  - [Linker](#linker-1)
  - [Bytecode Format](#bytecode-format-vhb)
    - [File Structure](#file-structure)
    - [Header Fields](#header-fields)
    - [Line Numbers Table](#line-numbers-table)
    - [Instruction Format](#instruction-format)
    - [Address decoding for jump/call instructions](#address-decoding-for-jumpcall-instructions)
    - [Address encoding](#address-encoding)
    - [Constants Table](#constants-table)
    - [Strings Table](#strings-table)
    - [Globals Metadata](#globals-metadata)
    - [VHB File Size Calculation](#vhb-file-size-calculation)
    - [Complete Example](#complete-example)
    - [Parsing the file](#parsing-the-file)
    - [Loading Process](#loading-process)
  - [Virtual Machine](#virtual-machine)
- [Example Program](#-example-program)
- [Debug Mode](#debug-mode)
  - [Debugger Commands](#debugger-commands)
  - [Debugger Features](#debugger-features)
  - [Debugger Display](#debugger-display)
- [Line Numbers in Errors](#line-numbers-in-errors)

---

**VHG** is a small, self‑contained programming language that compiles to a custom **register‑based bytecode** and runs on a **virtual machine**. The entire toolchain is written in modern C++ and demonstrates a complete compiler pipeline: lexical analysis, recursive‑descent parsing with operator precedence, an abstract syntax tree (AST), constant folding optimizations, and a RISC‑inspired instruction set.

---

## Highlights

- **Full compiler pipeline** – Lexer → Tokenizer → Parser → AST → Compiler → Bytecode → VM.
- **Rich language features** – variables (global/local), block scoping, `if`/`else`, `while`, `for` loops, functions with parameters and return values.
- **Strong typing for numbers and strings** – arithmetic, bitwise, logical, and comparison operators; string concatenation; **mutable string indexing** (`s[i]` read/write).
- **Arrays** – ordered, heterogeneous, reference‑typed collections with literal syntax (including matrices/nested arrays), indexing, and mutating helpers (`array_push`, `array_pop`, `array_insert`, `array_remove`).
- **Optimizations** – constant folding, implicit multiplication, post‑order code generation.
- **Standalone bytecode** – binary `.vhb` files with a `VHB1` magic header, loadable and executable by the VM without re‑parsing.
- **Clean, modular C++20** – extensive use of standard library, smart pointers, and RAII.

---

## Project Structure

| Directory / File    | Purpose                                                                   |
| ------------------- | ------------------------------------------------------------------------- |
| `main.cpp`        | CLI entry point – compile, run, or directly execute`.vhg` files.       |
| `Lexer/`          | Character‑by‑character input stream handling.                           |
| `Tokenizer/`      | Converts characters into tokens (keywords, operators, literals).          |
| `SymbolTable/`    | Manages variable scopes, stack offsets, and global addresses.             |
| `AST/`            | AST node definitions and the`OpCode` enumeration.                       |
| `Parser/`         | Recursive‑descent parser with shunting‑yard expression handling.        |
| `Compiler/`       | Transforms AST into bytecode; performs constant folding.                  |
| `VirtualMachine/` | Executes bytecode; includes register file, memory, and call stack.        |
| `Linker/`         | Merges compiled object units; resolves cross-unit calls and global slots. |

---

## Building the Compiler

### Requirements

- C++20 compatible compiler (g++ ≥ 11, clang ≥ 14, or MSVC 2022).
- Standard library with filesystem support.
- Simply run `runner.{ext}` on your OS.

### Runner files

#### Linux, MacOS or WSL

> Compilers: `g++` or `clang++`

---

```bash
g++ -std=c++20 -O3 *.cpp -o vhg
# or with static linking (Linux)
g++ -std=c++20 -O3 -static *.cpp -o vhg
```

```sh
#!/bin/bash

set -e

SOURCES="AST/ast.cpp Compiler/compiler.cpp Lexer/lexer.cpp Parser/parser.cpp Runner/main.cpp Tokenizer/tokenizer.cpp VirtualMachine/vm.cpp VirtualMachine/debugger.cpp VirtualMachine/ntprinter.cpp"

CXX="g++"
CXXFLAGS="-std=c++20 -O3"

echo "Compiling VHG..."

$CXX $CXXFLAGS $SOURCES -o vhg

if [ $? -eq 0 ]; then
    echo ""
    echo "[OK] Build successful!"
    echo ""
    echo "Usage:"
    echo "  ./vhg program.vhg"
    echo "  ./vhg compile input.vhg output.vhb"
    echo "  ./vhg run program.vhb [--debug]"
else
    echo ""
    echo "[ERROR] Build failed!"
    echo "Check the error messages above."
fi
```

```shell
chmod +x runner.sh
bash ./runner.sh
```

#### Windows

> Compiler: `cl.exe` (MSVC)

> Required environment: `Developer Command Prompt for Visual Studio 2022`

---

```cmd
cl /EHsc /O2 /std:c++20 *.cpp /Fe:vhg.exe
```

```bat
@echo off

@REM Find Visual Studio installation automatically
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set VS_PATH=%%i
)
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64

setlocal enabledelayedexpansion

set SOURCES=..\Language\AST\ast.cpp ..\Language\Compiler\compiler.cpp ..\Language\Lexer\lexer.cpp ..\Language\Parser\parser.cpp ..\Language\Runner\main.cpp ..\Language\Tokenizer\tokenizer.cpp ..\Language\VirtualMachine\vm.cpp ..\Language\VirtualMachine\debugger.cpp ..\Language\VirtualMachine\printer.cpp ..\Language\Linker\linker.cpp

set CXX=cl
set CXXFLAGS=/EHsc /O2 /std:c++20 /W3

echo Compiling VHG...

%CXX% %CXXFLAGS% %SOURCES% /Fe:vhg.exe

if %errorlevel% equ 0 (
    echo.
    echo [OK] Build successful!
    echo.
    echo Usage:
    echo "./vhg.exe <file.vhg>                               Build and run source directly"
    echo "./vhg.exe comple <in.vhg> [out.vhb]                Compile to bytecode"
    echo "./vhg.exe compile-obj <in.vhg> [out.vhb]           Compile to linkable object"
    echo "./vhg.exe link <a.vhb> [b.vhb ...] -o out.vhb      Link objects"
    echo "./vhg.exe run <in.vhb> [--debug]                   Run bytecode"
) else (
    echo.
    echo [ERROR] Build failed!
    echo Check the error messages above.
)

endlocal
```

```powershell
./runner.bat
```

> **Note:** The project does not depend on any external libraries beyond the C++ standard library.

---

## Usage

The executable `vhg` (or `vhg.exe`) accepts three modes:

### 1. Run source directly (backward‑compatible)

```bash
./vhg program.vhg
```

Parses, compiles, and executes the `.vhg` source in one step.

### 2. Compile to bytecode

```bash
./vhg compile input.vhg [output.vhb]
```

If no output path is given, it defaults to `input.vhb`.

### 3. Run pre‑compiled bytecode

```bash
./vhg run program.vhb
```

Loads the `.vhb` binary and executes it on the VM.

### 4. Compile to object unit

```bash
./vhg compile-obj input.vhg [output.vhb]
```

Compiles a `.vhg` source file into a **linkable object unit** — cross-unit function calls are left unresolved and no `main` function is required. If no output path is given, defaults to `input.vhb`.

### 5. Link object units

```bash
./vhg link a.vhb b.vhb lib.vhb -o program.vhb
```

Merges one or more `.vhb` object units produced by `compile-obj` into a single executable `.vhb`. The `-o` flag is required. The result can be run with `./vhg run program.vhb` as normal.

---

## Linker

The linker combines multiple independently compiled object units (`.vhb` files) into a single executable bytecode file. This is the proper alternative to `import` for larger projects where you want to compile modules separately and reuse them without full recompilation.

### When to use the linker vs. `import`

|                          | `import "file.vhg"`            | Linker (`compile-obj` + `link`)           |
| ------------------------ | -------------------------------- | --------------------------------------------- |
| How it works             | Text substitution before parsing | Each file compiled independently, then merged |
| Cross-file globals       | Shared (merged at source level)  | Shared by name across units                   |
| Cross-file functions     | Inlined at compile time          | Resolved at link time                         |
| Recompile on change      | Full recompile every time        | Only recompile the changed unit               |
| Multiple`main` allowed | No                               | No (exactly one across all units)             |

> **Note:** The `import` statement still works as before. The linker is an optional separate pipeline for larger projects.

### Typical multi-file workflow

```bash
# 1. Compile each module into an object unit
./vhg compile-obj math_utils.vhg
./vhg compile-obj physics.vhg
./vhg compile-obj main_module.vhg   # this file defines main()

# 2. Link all units into one executable
./vhg link math_utils.vhb physics.vhb main_module.vhb -o game.vhb

# 3. Run
./vhg run game.vhb
```

Only the file containing `main()` defines the entry point; all others are pure library units.

If only one file changes, only that file needs to be recompiled:

```bash
./vhg compile-obj main_module.vhg   # only this changed
./vhg link math_utils.vhb physics.vhb main_module.vhb -o game.vhb
```

### Rules and error conditions

- Exactly **one** `main` function must be defined across all linked units. Missing or duplicate `main` is a link-time error.
- **Function names must be unique** across all units. Defining the same function in two units is a link-time error.
- Global variables with the **same name** in different units share one memory slot. Declaring `score = 0` in two files that are linked together is fine — they refer to the same variable.
- Cross-unit calls to functions not defined in any unit produce a link-time error naming the missing function.
- Global slot indices are stored in 8 bits in `STORE_VAR`; the linker enforces a limit of **255 total global slots** across all linked units.

---

## Language Syntax Overview

### Variables & Scoping

- **Global** variables persist throughout the program.
- **Local** variables are declared inside blocks (including function bodies) and use stack‑based allocation.
- Use the `local` or `global` keyword to explicitly control storage; otherwise, the parser defaults to **local** inside any block and **global** at the top level.
- The `variable` (or `var`) explicitly declares a variable with automatic scope detection.

```vhg
# # Variable declaration keywords
variable x = 10; # explicit declaration (auto scope)
var y = 20; # 'var' is alias for 'variable'
global counter = 0; # explicit global (Not recommended)
local temp = 42; # # explicit local

# # Declaration without initializer (defaults to 'none')
variable z;

# Redeclaration in same scope is NOT allowed
variable a = 5;

variable a = 10;      # Error: Variable redefinition

# Block scoping
for (i = 0; i < 10; i += 1) {
    local square = i * i;    # block‑scoped local
    var cube = i * i * i;    # also block‑scoped
    print(square, cube, "\n");
}
# square and cube are out of scope here

```

### Variable Declaration Rules

| Syntax                                   | Scope                                            | Notes                         |
| ---------------------------------------- | ------------------------------------------------ | ----------------------------- |
| `variable x = expr;`                   | Auto (local in blocks, global at top-level)      | Explicit declaration          |
| `var x = expr;`                        | Auto (same as`variable`)                       | Short form alias              |
| `local variable x = expr;`             | Local                                            | Forces local storage          |
| `local var x = expr;`                  | Local                                            | Short form with local         |
| `global variable x = expr;`            | Global                                           | Forces global storage         |
| `global var x = expr;`                 | Global                                           | Short form with global        |
| `x = expr;`                            | Auto (implicit declaration if new)               | Assignment/declaration        |
| `x;`                                   | Auto (implicit declaration with`none`)         | Declaration-only              |
| `global/local var a,b,c = 3,4,5`       | Equvalent to`var a=3; var b=4; var c=5;`       | Multiple variable declaration |
| `global/local var a=5, b=4, c="Hello"` | Equvalent to`var a=5; var b=4; var c="Hello";` | Multiple variable declaration |

***Important Rules:***

- Variables cannot be redeclared in the same scope
- Local variables shadow globals with the same name
- `local` is not allowed in top-level (global) scope
- Implicit declarations follow the same scope rules as explicit ones
- All variables default to `none` if not explicitly initialized
- For `Multiple variable declaration` the keyword `variable` (`var`) is required
- The `Multiple variable declaration` follows `single variable declaration` rules

### It is desirable to know

The keywords `var` and `variable` are iterchangeable. You can use either form.

***Basic variable declarations***

```
variable x = 10; # explicit declaration (auto scope)
var y = 20; # 'var' is alias for 'variable'
local a = 5; # explicit local variable
global b = "Hello"; # explicit global variable
```

***The order of keywords is flexible***

```
# # These are valid and equivalent:
variable local x = 10;
local variable x = 10;

variable global y = 10;
global variable y = 10;

var local z = 23;
local var z = 23;

for(var local i = 0;i<4;i+=1) { #* ... *# }
for(local var i = 0;i<4;i+=1) { #* ... *# }

for(global var i = 0;i<4;i+=1) { #* ... *# }
for(var global i = 0;i<4;i+=1) { #* ... *# }

for(variable local i = 0;i<4;i+=1) { #* ... *# }
for(local variable i = 0;i<4;i+=1) { #* ... *# }

for(global variable i = 0;i<4;i+=1) { #* ... *# }
for(variable global i = 0;i<4;i+=1) { #* ... *# }
```

---

### Data Types

- **Numbers** – double‑precision floating point (internally `double`).
- **Strings** – double‑ or single‑quoted literals; supports escape sequences `\n`, `\t`, `\"`, `\\`. Strings are **mutable**: characters can be read and written by index (see [String Indexing](#string-indexing)).
- **Booleans** – `true` and `false` are stored as `1.0` and `0.0`.
- **Arrays** – ordered, heterogeneous, **reference‑typed** collections created with `[...]` literals or `array(n)`. See [Arrays](#arrays).
- **None** – `none` represents the absence of a value (`std::monostate`).

### String Indexing

Strings support subscript syntax for reading and writing individual characters. Indices are **0‑based**; both read and write use the same `[]` syntax.

**Read** (expression) — returns a **single‑character string**:

```vhg
var str = "hello";
var ch = str[2];     # ch is "l"
print(ch, "\n");
```

**Write** (statement) — modifies the string **in place**:

```vhg
var message = "world";
message[0] = 'W';    # message becomes "World"
message[4] = 'd';    # no visible change (already 'd')
```

**Chained reads** are supported (each step yields a one‑character string):

```vhg
var s = "abc";
print(s[0][0], "\n");   # "a"
```

**Assignment target** for writes must be a **variable** with a single index (e.g. `msg[i] = 'x'`). Chained assignment such as `msg[0][1] = 'a'` is not supported.

| Rule           | Behavior                                                        |
| -------------- | --------------------------------------------------------------- |
| Index type     | Must be a number; non‑integers are a runtime error             |
| Bounds         | `0 <= index < length(s)`; out of range → runtime error       |
| Object         | Must be a string; otherwise → runtime error                    |
| Assigned value | Must be a string of**length 1** (e.g. `'W'` or `"x"`) |
| Empty string   | No valid index; any access is out of bounds                     |

**Example program** (`vhg_files/string_index.vhg`):

```vhg
void function main() {
    var message = "world";
    message[0] = 'W';
    message[4] = 'd';
    print(message[0], "\n");    # W
    print(message);             # World
}
```

**Implementation (pipeline):**

| Stage     | Role                                                |
| --------- | --------------------------------------------------- |
| Tokenizer | `[` `]` tokens                                  |
| AST       | `SubscriptReadNode`, `SubscriptWriteNode`       |
| Parser    | `x[y]` in expressions; `x[y] = z` as assignment |
| Compiler  | `LOAD_STR_IDX`, `STORE_STR_IDX`                 |
| VM        | Bounds and type checks; in‑place mutation on write |

Opcodes **100** (`LOAD_STR_IDX`) and **101** (`STORE_STR_IDX`) — see `Language/AST/OpCodes.md`. These two opcodes are shared with array indexing (see [Arrays](#arrays)): the VM dispatches on the runtime type of the base value, so the same instructions handle `str[i]` and `arr[i]`.

### Arrays

VHG arrays are ordered, heterogeneous, **reference‑typed** collections. Assigning an array to another variable, passing it to a function, or nesting it inside another array all share the **same underlying storage** — mutating it through any one of those references is visible everywhere else that reference is held (the same model used by arrays/lists in most scripting languages, and unlike VHG's value‑typed strings).

**Creating arrays**

```vhg
var arr = array(5);        # a new array of size 5, every slot is `none`
var new_arr = array(1,2,3,"hello"); # a new array: [1,2,3,"hello"]
var numbers = [1, 2, 3, 4, 5];
var mixed = [10, 'hello', 3.14, none];   # elements may be any mix of types
```

**Matrices / nested arrays**

Array literals can themselves contain array literals, giving matrices or arbitrarily nested arrays:

```vhg
var matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];
print(matrix[1][2]);   # 6
matrix[0][1] = 99;      # mutates that row in place
```

**Reading and writing elements**

Indices are **0‑based** and use the same `[]` syntax as strings:

```vhg
var a = [10, 20, 30];
a[1] = 99;          # updating -> a = [10, 99, 30]
var x = a[0];       # reading  -> x = 10
print(a[2]);         # 30
```

Unlike string assignment, the base of an array assignment doesn't have to be a plain variable — chained subscripts work too, which is what makes `matrix[i][j] = x;` possible.

**Size	**

`length()` works on arrays the same way it works on strings:

```vhg
var len = length(a);   # 3
```

**Mutating helpers**

All four helpers mutate the array **in place**; you never need to reassign the result back to the variable.

| Function                            | Effect                                                        | Returns             |
| ----------------------------------- | ------------------------------------------------------------- | ------------------- |
| `array_push(arr, value)`          | Appends`value` to the end of `arr`                        | The new length      |
| `array_pop(arr)`                  | Removes and returns the last element of`arr`                | The removed element |
| `array_insert(arr, index, value)` | Inserts`value` at `index`, shifting later elements right  | The inserted value  |
| `array_remove(arr, index)`        | Removes the element at`index`, shifting later elements left | The removed element |

```vhg
var a = [10, 20, 30];
array_push(a, 40);          # a = [10, 20, 30, 40]
var last = array_pop(a);    # last = 40, a = [10, 20, 30]
array_insert(a, 1, 50);     # a = [10, 50, 20, 30]
array_remove(a, 2);         # a = [10, 50, 30]
```

Because arrays are reference types, functions can mutate an array passed to them without returning anything:

```vhg
void function fill(target) {
    array_push(target, 1);
    array_push(target, 2);
}

void function main() {
    var a = [];
    fill(a);
    print(a);   # [1, 2]
}
```

**Concatenation and repetition**

Arrays also support the same `+` (concatenation) and `*` (repetition) operators as strings — both build a **brand‑new array** and leave the operands untouched, unlike the in‑place mutating helpers above:

```vhg
var a = [1, 2, 3];
var b = [4, 5];
print(a + b);        # [1, 2, 3, 4, 5]
print(a);             # [1, 2, 3]  (unchanged)

print([1, 2] * 3);    # [1, 2, 1, 2, 1, 2]
print(3 * [1, 2]);    # [1, 2, 1, 2, 1, 2]  (either operand order works)
```

Since `arr += other` desugars to `arr = arr + other` (the same rule VHG already uses for numbers and strings), `+=` works too — it just reassigns `arr` to the newly concatenated array rather than mutating the original in place:

```vhg
var d = [1, 2];
d += [3, 4];
print(d);   # [1, 2, 3, 4]
```

`+` requires **both** operands to be arrays (an array plus a non‑array is a runtime error — use `array_push` to append a single element instead). `*` requires one operand to be an array and the other a non‑negative number.

**Rules**

| Rule              | Behavior                                                                 |
| ----------------- | ------------------------------------------------------------------------ |
| Index type        | Must be a number; non‑integers are a runtime error                      |
| Bounds            | `0 <= index < length(arr)`; out of range → runtime error              |
| `array(n)` size | Must be a non‑negative integer                                          |
| Equality (`==`) | Compares by reference (same underlying array), not element‑by‑element  |
| Truthiness        | An empty array is falsy; a non‑empty array is truthy                    |
| `type(arr)`     | Returns`"array"`                                                       |
| `+`             | Array + array → new concatenated array. Array + non‑array is an error. |
| `*`             | Array\* non‑negative number (either order) → new repeated array.       |
| `*=`            | Same as the`arr = arr * number`                                        |
| `+=`            | Same as`arr = arr + other`; reassigns rather than mutating in place    |

**Example program** (`vhg_files/arrays.vhg`):

```vhg
void function main() {
    var a = [10, 20, 30];
    a[1] = 99;
    print(a);              # [10, 99, 30]

    array_push(a, 40);
    print(a);              # [10, 99, 30, 40]

    var last = array_pop(a);
    print(last);            # 40
    print(a);              # [10, 99, 30]

    array_insert(a, 1, 50);
    print(a);              # [10, 50, 99, 30]

    array_remove(a, 2);
    print(a);              # [10, 50, 30]
}
```

**Implementation (pipeline):**

| Stage     | Role                                                                                                                                                                                                                                                                                                                                      |
| --------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Tokenizer | `[` `]` tokens;`array`, `array_push`, `array_pop`, `array_insert`, `array_remove` recognized as builtin names                                                                                                                                                                                                               |
| AST       | `ArrayLiteralNode`;`SubscriptReadNode` / `SubscriptWriteNode` are shared with string indexing                                                                                                                                                                                                                                       |
| Parser    | `[e1, e2, ...]` as a primary expression (including nested/matrix literals);`x[y] = z` where the base may be a chained subscript                                                                                                                                                                                                       |
| Compiler  | `ARRAY_NEW`, `ARRAY_LIT`, `ARRAY_PUSH`, `ARRAY_POP`, `ARRAY_INSERT`, `ARRAY_REMOVE`; `LOAD_STR_IDX`/`STORE_STR_IDX` generalized to dispatch on runtime type; `ADD`/`MUL` generalized for `+` (concatenation) and `*` (repetition) — no new opcodes needed since `+=` already desugars to `arr = arr + other` |
| VM        | `Value` gains a`shared_ptr<ArrayObj>` alternative (reference semantics); bounds and type checks on every array operation                                                                                                                                                                                                              |

### Operators

| Category               | Operators                                                                                                                                                                                                                                                            |
| ---------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Arithmetic             | `+` `-` `*` `/` `%` `//` (floor) `%/` (fractional) `**`                                                                                                                                                                                              |
| Bitwise                | `&` `\|` `^` `<<` `>>` `~`                                                                                                                                                                                                                                |
| Logical                | `and` `or` `not`                                                                                                                                                                                                                                               |
| Comparison             | `==` `!=` `<` `>` `<=` `>=`                                                                                                                                                                                                                              |
| Assignment             | `=` `+=` `-=` `*=` `/=` `%=` `^=` (including on subscripts, e.g.`arr[i] += 1;`)                                                                                                                                                                      |
| String                 | `+` `+=` (concatenation), `length(s)` → size, `*` → repetition, `s[i]` read, `s[i] = c` write                                                                                                                                                          |
| Array                  | `arr[i]` read, `arr[i] = x` write (including chained, e.g.`matrix[i][j] = x`), compound assignment (e.g.`arr[i] += 1;`), `length(arr)` → size, `+` → concatenation (new array), `*` → repetition (new array), `+=` → reassigns to`arr + other` |
| Ternary                | `condition ? trueBranch : falseBranch` (e.g `x = 5 > 6 ? 7 : 8;`)                                                                                                                                                                                                |
| Loop operators         | `break;` -> exit loop earlier, `continue;` -> skip next iteration                                                                                                                                                                                                |
| **Declarations** | `variable`, `var`, `local`, `global`                                                                                                                                                                                                                         |

> **Note:** `and` / `or` **short-circuit**, like most languages. In `left and right`, `right` is only evaluated when `left` is truthy; in `left or right`, `right` is only evaluated when `left` is falsy. This matters for guarded expressions such as `i >= 0 and arr[i] > 0` — when `i` is `-1`, `arr[i]` is never touched. Both operators always evaluate to `1.0` or `0.0` (not the operand's own value, unlike Python's `and`/`or`).

> **Note:** Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`, `^=`) works on subscript targets too, not just plain variables: `arr[i] += 1;` reads `arr[i]`, applies the operator, and writes the result back to `arr[i]` — equivalent to `arr[i] = arr[i] + 1;`.

### Mathematical Functions

| Function                 | Description                                                                                         | Number of Arguments |
| ------------------------ | --------------------------------------------------------------------------------------------------- | ------------------- |
| `sin(x)`               | Sine                                                                                                | 1                   |
| `cos(x)`               | Cosine                                                                                              | 1                   |
| `tan(x)`               | Tangent                                                                                             | 1                   |
| `asin(x)`              | Arc sine                                                                                            | 1                   |
| `acos(x)`              | Arc cosine                                                                                          | 1                   |
| `atan(x)`              | Arc tangent                                                                                         | 1                   |
| `atan2(y, x)`          | Arc tangent (two arguments)                                                                         | 2                   |
| `sqrt(x)`              | Square root                                                                                         | 1                   |
| `cbrt(x)`              | Cube root                                                                                           | 1                   |
| `pow(x, y)`            | Power (x^y)                                                                                         | 2                   |
| `exp(x)`               | Exponential (e^x)                                                                                   | 1                   |
| `log(x)`               | Natural logarithm (base e)                                                                          | 1                   |
| `ln(x)`                | Natural logarithm (base e)                                                                          | 1                   |
| `log10(x)`             | Base-10 logarithm                                                                                   | 1                   |
| `log2(x)`              | Base-2 logarithm                                                                                    | 1                   |
| `log_ab(a, b)`         | Logarithm of`b` with base `a` (log(b)/log(a))                                                   | 2                   |
| `ceil(x)`              | Round up                                                                                            | 1                   |
| `floor(x)`             | Round down                                                                                          | 1                   |
| `round(x)`             | Round to nearest integer                                                                            | 1                   |
| `abs(x)`               | Absolute value                                                                                      | 1                   |
| `fmod(x, y)`           | Floating-point remainder                                                                            | 2                   |
| `random(min=0, max=1)` | Returns a random floating-point number in the range**[min, max)**. Default range is**[0, 1)** | 0 or 2              |

---

### Mathematical Constants

| Constant  | Value             | Description        |
| --------- | ----------------- | ------------------ |
| `m_pi`  | 3.141592653589793 | π (Pi)            |
| `m_e`   | 2.718281828459045 | e (Euler's number) |
| `m_max` | 1.79769e+308      | The Maximum Number |
| `m_inf` | inf               | The Infinity       |

> **Note:** Trigonometric functions use radians by default.
> For degrees use this formula: `argument*m_pi/180`. E.g. `sin(60*m_pi/180)`.

---

### Control Flow

```vhg
void function main() {
    variable x = input("x= ");
    if (x > 0) {
        print("positive\n");
    } else if (x < 0) {
        print("negative\n");
    } else {
        print("zero\n");
    }

    while (n > 0) {
        n -= 1;
    }

    for (i = 0; i < 5; i += 1) {
        print(i, " ");
    }
}
```

### Functions

```vhg
# # non-void function requires "return"
function add(a, b) {
    return a + b;
}

void function foo() {
    print("Hello world");
}

void function main() {
    result = add(10, 20);
    foo();
    print(result);
}
```

- Parameters are passed by value. Arrays are, themselves, reference values (see [Arrays](#arrays)) — passing an array parameter copies the reference, not the elements, so mutations inside the function are visible to the caller.
- Functions can be called before they are defined (forward declaration via bytecode patching).
- Functions can call themselves (recursion) — each call gets its own frame on the call stack.

#### Default arguments

A parameter can declare a default value with `name = expr`. Callers may omit any trailing argument that has a default; the default expression is evaluated (fresh, every call) when the caller didn't supply that argument.

```vhg
function greet(name = "World") {
    print("Hello, "); print(name); print("!\n");
}

function add(a, b = 10) {
    return a + b;
}

void function main() {
    greet();          # Hello, World!
    greet("Claude");  # Hello, Claude!

    print(add(1, 2)); # 3
    print(add(5));    # 15 - uses default b = 10
}
```

Rules:

- Once a parameter has a default, every parameter after it (aside from a trailing `*args`) must also have a default.
- Calling with too few arguments (fewer than the required, non-default parameters) is a compile-time error; so is passing more arguments than the function declares (unless it takes `*args`, see below).
- A default expression is compiled into the callee, not the caller, and evaluated in the callee's own scope — a bare name inside a default resolves as a **global**, not as another parameter. `function f(a, b = a)` will *not* make `b` default to `a`'s value.

#### Variadic parameters (`*args`)

A function's **last** parameter may be prefixed with `*` to collect any extra positional arguments into an ordinary array, similar to Python's `*args`:

```vhg
function sum_all(*nums) {
    var total = 0;
    var i = 0;
    while (i < length(nums)) {
        total = total + nums[i];
        i = i + 1;
    }
    return total;
}

void function main() {
    print(sum_all(1, 2, 3));        # 6
    print(sum_all(10, 20, 30, 40)); # 100
    print(sum_all());               # 0 - no args at all is fine, nums = []
}
```

`*args` can be combined with regular (and defaulted) parameters, as long as it comes last:

```vhg
void function describe(label, *items) {
    print(label); print(" got "); print(length(items)); print(" extra item(s): ");
    print(items);
    print("\n");
}

function foo(a = 3, *args) {
    return a;
}

void function main() {
    describe("scores", 90, 85, 77); # scores got 3 extra item(s): [90, 85, 77]
    describe("empty");              # empty got 0 extra item(s): []

    print(foo());        # 3 (default a, args = [])
    print(foo(99, 1, 2)); # 99 (a = 99, args = [1, 2])
}
```

Rules:

- Only one `*args` parameter is allowed per function, and it must be the last parameter — nothing (named or defaulted) may follow it.
- Inside the function, the variadic name is a normal local array variable: `length(...)`, indexing, and the array helpers (`array_push`, etc.) all work on it.
- A call with `*args` present has no upper limit on argument count; it still must supply at least the function's non-default, non-variadic parameters.

### Program entry (`main`)

Every complete program must define exactly one entry function:

```vhg
void function main() {
    print("Hello, world\n");
}
```

Rules:

- `main` must be defined at **top level** (not nested inside another function).
- `main` must take **no parameters** (not even `*args` or defaulted ones).
- Only one `main` is allowed per program (after `import` merging).
- At top level you may declare **globals** and **other functions**; executable statements (`print`, `if`, loops, calls, etc.) belong inside `main` (or another function).
- Global initializers (for example `counter = 0;`) still run before `main` is called, similar to C++.

### Switch/case

```
x = 2;
switch(x) {
    case 1:
        print("One\n");
        break;
    case 2, 3, 4:
        print("Two, Three or Four\n");
        break;
    case 5:
        print("Five\n");
        break;
    default:
        print("Other\n");
}
```

### Built‑in I/O

- `input(prompt1, prompt2, ...)` - User Input.

  | Input                                                   | Type       | Description                          |
  | ------------------------------------------------------- | ---------- | ------------------------------------ |
  | `"123"` or `'123'`                                  | `string` | Will return`123` as string         |
  | `123`                                                 | `number` | Will return`123` as number         |
  | Empty input                                             | `none`   | Will return`none` (std::monostate) |
  | `Hello World`, `"Hello World"` or `'Hello World'` | `string` | Will return`Hello World`           |


  > The `input` function automatically determines the type of the entered value and removes any quotation marks (`""` or `''`) if they were included.
  >
- `print(expr1, expr2, ...)` – prints each argument; automatically appends a newline **if only one argument is given** (otherwise you must include `"\n"` explicitly). Arrays print as `[e1, e2, ...]`, with nested arrays and quoted strings rendered recursively.
- `length(string | array)` - returns the size of a given string or array.
- `type(argument)` - Returns the type of given argument (`string`, `number`, `array` or `none`).
- `chr(number)` - Returns a Unicode string of one character with code point i; `0 <= i <= 0x10FFFF`.
- `ord(string)` - Returns the Unicode code point for the first character of a string.
- `bin(integer)` - Return the binary representation of an integer.
- `oct(integer)` - Return the octal representation of an integer.
- `hex(integer)` - Return the hexadecimal representation of an integer.
- `dec(string)` - Returns the decimal representation of given argment (if possible).
- `array(size)` - Returns a new array of `size` elements, each initialized to `none`. See [Arrays](#arrays).
- `array(arg1, arg2, arg3, ...)` - Returns a new array with `[arg1, arg2, arg3, ...].`
- `array_push(arr, value)` - Appends `value` to `arr` in place; returns the new length.
- `array_pop(arr)` - Removes and returns the last element of `arr` in place.
- `array_insert(arr, index, value)` - Inserts `value` at `index` in `arr` in place; returns the inserted value.
- `array_remove(arr, index)` - Removes and returns the element at `index` from `arr` in place.
- `number(string)` - Number cast.
- `string(argument)` - String cast.

---

### The syntax of number types.

| Syntax            | Example            | Will understand as   |
| ----------------- | ------------------ | -------------------- |
| `0b`, `0B`    | `0b1100`         | BIN                  |
| `0o`, `0O`    | `0o45`           | OCT                  |
| `0x`, `0X`    | `0xff`           | HEX                  |
| `_` separator   | `1_000_000`      | `1000000`          |
| `e±N` `E±N` | `2e+3`, `3E-4` | `2000`, `0.0003` |

> **Note** Implicit multiplication (`3x` as `3*x`) won't work for variables named `e` and `E`, as `3e4` will always be parsed as `3×10⁴ = 30000`.

> **Note** Implicit multiplication works for variables too.
> E.g. `x z` will understand as `x * z`.

> **Note** `_` is stripped silently as a visual separator. `e`/`E` are
> processed as scientific notation. Unlike `0b`/`0o`/`0x`, none of these
> require quotes in string context.

> **Note:** Negative numbers are written with a leading `-`, e.g. `"-0b1100"` is `-12`.
> **Note:** type of `binary`, `octal` or `hexadecimal` numbers is `string`. So don't forget `""` or `''`.

> **Note:** For `num < 0` case, `bin(num)`, `oct(num)` and `hex(num)` return the two's complement of `num` (32-bit).

### Import preprocessing

`import "path_to_file"`

- This allows to import functions and variables (global) from other files.

> **Note:** `import "file.vhg"` works as a text-level include (equivalent to `#include` in C++). For projects with multiple independently compiled files, use `compile-obj` + `link` instead — see [Linker](#linker).

## Architecture Deep Dive

### Lexer & Tokenizer

- `Lexer` provides a stream interface with `peek()` and `advance()`.
- `Tokenizer` groups characters into tokens, skipping whitespace and comments (`# ...` and `#* ... *#`). Builtin function names (including `array`, `array_push`, `array_pop`, `array_insert`, `array_remove`) are matched case‑insensitively, the same as the math builtins.

### Parser

- Recursive descent for statements (`if`, `while`, `for`, `function`, `return`, blocks).
- **Shunting‑Yard algorithm** for expressions, respecting operator precedence and associativity.
- Implicit multiplication (e.g., `2x` or `(a+b)(c+d)`) is handled by injecting a `*` token when appropriate.
- **Constant folding** is performed *during parsing* to simplify the AST immediately.
- **String subscripts** — postfix `[expr]` builds `SubscriptReadNode`; `name[idx] = value` builds `SubscriptWriteNode` (variable base only).
- **Array literals** — a leading `[` in expression position parses an `ArrayLiteralNode` (`[e1, e2, ...]`), including nested literals for matrices. Array subscripts reuse `SubscriptReadNode`/`SubscriptWriteNode`, but assignment bases may be a chained subscript expression (e.g. `matrix[i][j] = x;`), not just a plain variable.

### Symbol Table

- Manages nested block scopes via a stack of `ScopeLevel` objects.
- Global variables are stored in a flat address space.
- Local variables receive negative offsets relative to the **frame pointer** (`FP` / `x8`).
- **Explicit declarations** (`variable`, `var` `local`, `global`) allocate fresh slots and prevent redeclaration.
- **Implicit declarations** (plain assignment) reuse existing slots or create new ones on-the-fly.
- Function definitions push a fresh scope stack, preserving outer scopes for later restoration.
- Declared but unassigned variables store `none` by default.

---

### Compiler

- Traverses the AST in post‑order, generating a linear sequence of `Instruction`s.
- Allocates virtual registers on‑the‑fly (except `x2` = SP, `x8` = FP).
- Emits function prologues/epilogues that adjust SP and FP.
- Patches forward function calls after all code is generated.
- **Constant folding** is re‑applied during optimization (redundant constants are merged).
- Outputs a `ByteCode` structure containing instructions, constant pool (numbers), and string pool.
- Emits **`LOAD_STR_IDX`** (read character/element) and **`STORE_STR_IDX`** (write character/element) — these two opcodes are shared between strings and arrays and dispatch on the runtime type of the base value at execution time.
- Emits **`ARRAY_NEW`** for `array(n)`, **`ARRAY_LIT`** + a sequence of **`ARRAY_PUSH`** for `[...]` literals (each element is appended to the new array immediately after it's evaluated, rather than staged through a shared buffer, so nested/matrix literals compile correctly), and **`ARRAY_PUSH`**/**`ARRAY_POP`**/**`ARRAY_INSERT`**/**`ARRAY_REMOVE`** for the corresponding builtin calls.

---

### Linker

- Accepts multiple `ByteCode` units produced by `compile-obj` (each compiled with `allowUnresolvedCalls = true`).
- Computes a **base offset** for each unit in the merged instruction stream and rebases all jump/call addresses accordingly.
- **Deduplicates constant and string pools** — identical `double` values and string literals across units are merged into one entry; `LOAD_CONST` / `LOAD_STR` / `PRINT_STR` indices are remapped.
- **Unifies global variable slots by name** — two units that declare a global with the same name share one slot in the merged output; `LOAD_VAR` / `STORE_VAR` addresses are remapped.
- **Resolves cross-unit calls** recorded in `ByteCode::unresolvedCalls` by patching the absolute address of each target function after all units are merged.
- Validates that exactly one `main` is defined and emits the final `CALL main` epilogue.
- Throws a descriptive `std::runtime_error` for duplicate function definitions, unresolved calls, missing `main`, or global slot overflow (> 255).
- Array opcodes (`ARRAY_NEW`, `ARRAY_LIT`, `ARRAY_PUSH`, `ARRAY_POP`, `ARRAY_INSERT`, `ARRAY_REMOVE`) carry only register operands — they don't reference the constant, string, or global pools — so linking requires no special handling for them.

| Step                | What happens                                                                                         |
| ------------------- | ---------------------------------------------------------------------------------------------------- |
| Address rebasing    | Each unit's`JMP`/`JZ`/`JNZ`/`CALL` targets are shifted by the unit's instruction base offset |
| Constant pool merge | Identical`double` constants deduplicated; `LOAD_CONST` indices remapped                          |
| String pool merge   | Identical string literals deduplicated;`LOAD_STR`/`PRINT_STR` indices remapped                   |
| Global slot merge   | Globals unified by name;`LOAD_VAR`/`STORE_VAR` addresses remapped                                |
| Call resolution     | Unresolved cross-unit calls patched with target's absolute address                                   |
| `main` epilogue   | `CALL main` appended after all units, same as single-file `compile`                              |

---

### Bytecode Format (`.vhb`)

The `.vhb` (VH Binary) file is a platform-independent binary representation of the compiled program.
It contains all inforamtion needed to execute the program without re-parsing the source code.

#### File Structure

```
+--------------------------------------------------+
| HEADER |
+--------------------------------------------------+
| Magic: "VHB1" | 4 bytes |
| Instruction Count (M) | 4 bytes |
| Constant Count (C) | 4 bytes |
| String Count (S) | 4 bytes |
| Line Numbers Count (L = M) | 4 bytes |
+--------------------------------------------------+
| LINE NUMBERS TABLE |
+--------------------------------------------------+
| Line Numbers (M entries) | M * 4 bytes |
+--------------------------------------------------+
| INSTRUCTION TABLE |
+--------------------------------------------------+
| Instructions (M entries) | M * 4 bytes |
+--------------------------------------------------+
| CONSTANTS TABLE |
+--------------------------------------------------+
| Constants (C entries) | C * 8 bytes |
+--------------------------------------------------+
| STRINGS TABLE |
+--------------------------------------------------+
| For each of S strings: |
| - Length (uint32) | 4 bytes |
| - UTF-8 data | length bytes |
+--------------------------------------------------+
| GLOBALS METADATA |
+--------------------------------------------------+
| Global Slot Count (G) | 4 bytes |
| For each of G slots: |
| - Name length (uint32) | 4 bytes |
| - Name data | length bytes |
+--------------------------------------------------+
```

#### Header Fields

| Offset | Field              | Size    | Description                                     |
| ------ | ------------------ | ------- | ----------------------------------------------- |
| 0      | Magic              | 4 bytes | File identifier:`'V'` `'H'` `'B'` `'1'` |
| 4      | Instruction Count  | 4 bytes | Number of bytecode instructions (M)             |
| 8      | Constant Count     | 4 bytes | Number of floating‑point constants (C)         |
| 12     | String Count       | 4 bytes | Number of string literals (S)                   |
| 16     | Line Numbers Count | 4 bytes | Number of line number entries (must equal M)    |

#### Line Numbers Table

Contains the source line number for each instruction (used for error reporting).

```text
[Line0] [Line1] [Line2] ... [LineM-1]
```

Each entry is a 4-byte (32-bit) unsigned integer.

#### Instruction Format

Each instruction is exactly **4 bytes** with the following layout:
Byte 0: Opcode (8 bits)
Byte 1: Destination Register (8 bits)
Byte 2: Left Operand / Address low byte (8 bits)
Byte 3: Right Operand / Address high byte (8 bits)

**C++ representation:**

```c++
struct Instruction {
    uint32_t op : 8; // Operation code
    uint32_t dst : 8; // Destination register
    uint32_t left : 8; // Left operand or address low byte
    uint32_t right : 8; // Right operand or address high byte
};
```

Array instructions reuse this same 3‑operand shape:

| Opcode            | `dst`                           | `left`                        | `right`      |
| ----------------- | --------------------------------- | ------------------------------- | -------------- |
| `ARRAY_NEW`     | result register (new array)       | size register                   | —             |
| `ARRAY_LIT`     | result register (new empty array) | —                              | —             |
| `ARRAY_PUSH`    | result register (new length)      | array register                  | value register |
| `ARRAY_POP`     | result register (removed value)   | array register                  | —             |
| `ARRAY_INSERT`  | value register                    | array register                  | index register |
| `ARRAY_REMOVE`  | result register (removed value)   | array register                  | index register |
| `LOAD_STR_IDX`  | result register                   | base register (string or array) | index register |
| `STORE_STR_IDX` | value register                    | base register (string or array) | index register |

#### Address decoding for jump/call instructions:

```c++
    uint16_t address = (inst.right << 8) | inst.left;  
```

#### Address encoding:

```c++
    inst.left = address & 0xFF;
    inst.right = (address >> 8) & 0xFF;
```

#### Constants Table:

Stores floating-point constant values (IEEE-754 double precision, 8 bytes each).

```text
[Constant0] [Constant1] ... [ConstantC-1]
```

#### Strings Table

Each string is stored as a length-prefixed UTF-8 sequence:

```text
For i = 0 to S-1:
    uint32_t length = strlen(strings[i])
    write(length)
    write(strings[i], length)
```

Example:
`Hello` -> 0x05 0x00 0x00 'H' 'e' 'l' 'l' 'o'

#### Globals Metadata

Stores global variable names for runtime error messages (detecting reads for uninitialized globals).

```text
Global Slot Count (G) = number of global variable slots

For slot = 0 to G-1:
    uint32_t nameLength = strlen(globalNames[slot])
    write(nameLength)
    write(globalNames[slot], nameLength)
```

> **Note:** Arrays are always built at runtime by `ARRAY_NEW`/`ARRAY_LIT`/`ARRAY_PUSH` instructions — there is no array literal pool in the `.vhb` format, so the file layout above is unaffected by array support.

#### VHB File Size Calculation

```math
    Size = 24 + 8×M + 8×C + ∑(4 + |s|) + ∑(4 + |n|) bytes
```

**Where:**

`M` = number of instructions

`C` = number of constants

`|s|` = length of each string

`|n|` = length of each global variable name

`∑` over all strings and global names

#### Complete Example

```vhg
    void function main() {
        global var x = 10;
        print("Hello");
    }
```

![alt text](./Readme-images/hex_example.png)

**Its size will be:**
M = 11; (run `.vhb` in `--debug` to see all instructions)

C = 1 (only `10`);
|s|₁ = 5 (`"Hello"`); |s|₂=1 (`"\n"` from `print` function)

|n| = 1 (only `x`.)

```math
    Size = 24 + 8(11+1) + (9+5) + 5 = 139 bytes
```

#### Parsing the file:

* Offset 0-3: Magic `"VHB1"`
* Offset 4-7: Instruction count=2
* Offset 8-11: Constant count=1
* Offset 12-15: String count=1
* Offset 16-19: Line numbers count=2
* Offset 20-27: Line numbers = [1,2]
* Offset 28-35: Instruction (2×4 bytes)
* Offset 36-43: Constant (10.0)
* Offset 44-52: String ("Hello")
* Offset 53-56: Global slot count=1
* Offset 57-61: Global name length=1, name = "x"

#### Loading Process

When the VM loads a `.vhb` file, it performs these steps:

1. **Read and validate header** - Check magic number
2. **Read line numbers** - Store the error reporting
3. **Read instructions** - Copy into `current_program` vector
4. **Read constants** - Copy into `current_constants` vector
5. **Read strings** - Copy into `current_strings` vector
6. **Read globals metadata** - Set up `vmGlobalNames` for runtime checks
7. **Initialize registers and memory** - Set SP, FP, allocate memory.

### Virtual Machine

- **Register file** – 256+ registers (indexed by `uint8_t`), with `x2` as stack pointer and `x8` as frame pointer.
- **Memory** – linear array of `Value`, a variant of `monostate` (none), `double`, `std::string`, and `std::shared_ptr<ArrayObj>` (arrays). Arrays are the only reference type: copying a `Value` that holds an array copies the shared pointer, not the elements, which is what gives arrays their in‑place mutation semantics.
- **Call stack** – saves return address, caller’s SP/FP, and argument buffer.
- **Instruction set** – includes RISC‑V inspired arithmetic (`ADD`, `SUB`, `AND`, …), control flow (`JMP`, `JZ`, `CALL`, `RETURN`), memory access (`LOAD`/`STORE` relative to FP), string/array indexing (`LOAD_STR_IDX`, `STORE_STR_IDX`), and array construction/mutation (`ARRAY_NEW`, `ARRAY_LIT`, `ARRAY_PUSH`, `ARRAY_POP`, `ARRAY_INSERT`, `ARRAY_REMOVE`).
- Debug mode (`VirtualMachine(true)`) prints the AST and a disassembly of the generated bytecode.

## Example Program

```vhg
# Loop and local scoping
sum = 0;

void function main() {
    for (i = 1; i <= 10; i += 1) {
        local square = i * i;
        sum += square;
    }
    print("Sum of squares 1..10 = ", sum, "\n");
}
```

Run it:

```bash
./vhg compile fact.vhg
./vhg run fact.vhb
```

```vhg
# Variable declaration examples
var count = 0;           # auto-detect scope (global at top-level)
variable total = 0;      # explicit declaration with var keyword

void function main() {
    for (i = 1; i <= 10; i += 1) {
        local square = i * i;   # explicit local variable
        var cube = i * i * i;   # auto-detected as local (inside block)
        total += square;
        count += 1;
    }
    print("Count: ", count, ", Total: ", total, "\n");
}
```

```vhg
# Building a small multiplication table with arrays
void function main() {
    var table = array(5);
    var i = 0;
    while (i < length(table)) {
        table[i] = [];
        var j = 0;
        while (j < 5) {
            array_push(table[i], (i + 1) * (j + 1));
            j += 1;
        }
        i += 1;
    }
    print(table);
}
```

---

### Debug Mode

To run your program step by step with the built-in debugger:

```bash
./vhg compile app.vhg
./vhg run app.vhb --debug
```

The debugger provides interactive control over program execution with the following features:

#### Debugger Commands

| Command              | Aliases            | Description                                   |
| -------------------- | ------------------ | --------------------------------------------- |
| `Enter` / `step` | `si`             | Execute next instruction (step into)          |
| `over`             | `so`             | Step over function calls                      |
| `out`              | `su`             | Step out of current function                  |
| `go` / `c`       | `continue`       | Continue execution until next breakpoint      |
| `br.add <addr>`    |                    | Set breakpoint at instruction address         |
| `br.rem <addr>`    |                    | Remove breakpoint at address                  |
| `br`               | `br.list`        | List all breakpoints                          |
| `r<n>`             | `reg <n>`        | Show register value (e.g.,`r0`, `r3`)     |
| `m<addr>`          | `mem <addr>`     | Show memory value (e.g.,`m100`, `m10000`) |
| `h`                | `help`           | Open Commands Menu                            |
| `q`                | `quit`, `exit` | Quit debugger                                 |

#### Debugger Features

**Breakpoints:**

- Set breakpoints at specific instruction addresses
- Multiple breakpoints supported
- List all active breakpoints
- Remove individual breakpoints

**Step Control:**

- **Step Into**: Execute one instruction, entering function calls
- **Step Over**: Execute until returning to current call depth
- **Step Out**: Execute until returning from current function

**State Inspection:**

- View register values (non-zero registers displayed automatically) — array‑valued registers display as `[e1, e2, ...]`
- Inspect memory at specific addresses
- See call stack depth
- Source line numbers shown when available

**Examples:**

```bash
# Set breakpoint at instruction 10
(dbg) br.add 10

# Step through a few instructions
(dbg) step
(dbg) step

# Check register 5
(dbg) r5

# Step over a function call
(dbg) over

# Continue execution
(dbg) c
```

#### Debugger Display

The debugger shows:

1. **Current instruction address** and source line number
2. **Next instruction** to be executed
3. **Call stack depth** (if inside functions)
4. **Active registers** (non-zero values only)

Example output:

```
=== DEBUG @ 15  (line 5) ===
  next: LOAD_CONST r3 <- const[0]
  call depth: 1
  regs: r0=42 r1=3.14 r3=10

─── Commands ─────────────────────────────────────────
  Enter / step   Execute next instruction (step into)
  over           Step over function calls
  out            Step out of current function
  go / c         Continue until next breakpoint
  br.add <n>     Set breakpoint at address n
  br.rem <n>     Remove breakpoint at address n
  br             List all breakpoints
  r<n>           Show register value (e.g., r0)
  m<n>           Show memory value (e.g., m100)
  q              Quit debugger
────────────────────────────────────────────────────
(dbg)
```

---

### Line Numbers in Errors

The VHG toolchain now reports **source line numbers** for both parse‑time and runtime errors.

### How it works

- **Lexer** tracks the current line while reading characters.
- **Tokenizer** attaches the line number to every token using `markTokenStart()`.
- **Parser** stores the line number in each `StatementNode`.
- **Compiler** records the line number for every generated bytecode instruction.
- **VM** catches runtime exceptions and shows the line number together with the bytecode address.

### Example

```vhg
x = none;     # # line 1
print(x + 4); # # line 2
```

**This is runtime error**

```shell
Error: Line 2: Cannot add with None
```

```vhg
x = 5;
print(x ** 5);
break; # # line 3
```

**This is parse-time error**

```shell
Error: Line 3: break statement outside of loop or switch
```

Array errors follow the same convention:

```vhg
void function main() {
    var a = [1, 2, 3];
    print(a[5]);   # line 3
}
```

```shell
Error: Line 3: Array index out of bounds
```

---
