# 🧮 Math Expression Compiler

A high-performance C++ mathematical expression evaluator that follows a full compiler pipeline: from lexical analysis to Abstract Syntax Tree (AST) construction, and finally to bytecode execution via a Register-based Virtual Machine.

---
## 🚀 Technical Highlights

This project implements a full compilation pipeline for mathematical expressions, following modern compiler design principles.

### 🏛️ Architecture & Design Patterns

* **Three-Stage Pipeline**:
1. **Lexical Analysis**: Custom `Lexer` and `Tokenizer` with state management.
2. **Syntax Analysis**: `Recursive Descent Parser` combined with the **Shunting-Yard algorithm** for operator precedence.
3. **Code Generation**: AST (Abstract Syntax Tree) transformation into linear **Three-Address Bytecode**.


* **AST (Abstract Syntax Tree)**: Implemented using **Polymorphism** and `std::shared_ptr` for automatic memory management and clear node hierarchy.
* **Virtual Machine (VM)**: A register-based VM that executes optimized bytecode instructions, mimicking a real CPU execution cycle.

### ⚡ Optimization Techniques

* **Constant Folding**: Expressions like `5 + 10` or unary operations like `---5` are evaluated during the compilation phase to reduce VM overhead.
* **Implicit Multiplication**: Supports for mathematical shorthand like `5x` or `2(a+b)` through look-ahead        parsing logic.
* **RISC-style Instruction Set**: Purposefully chosen atomic instructions (e.g., `LOAD` + `NEG` instead of complex `LOAD_NEG`) to maintain architectural clarity and execution predictability.
* **Efficient Symbol Mapping**: Fast variable lookup using a `SymbolTable` that maps variable names to memory addresses before execution.

### 🛠️ Memory & Performance

* **Bytecode Linearization**: The AST is flattened into a `std::vector<Instruction>`, which provides high **Cache Locality** during VM execution compared to traversing a tree structure.
* **RAII (Resource Acquisition Is Initialization)**: Clean handling of objects and memory using modern C++ smart pointers.
---

## 📁 Project Structure

### 🔍 Lexical Analysis (Lexer & Tokenizer)

This layer is responsible for scanning the raw input string and breaking it into meaningful units.

* **`lexer.h / lexer.cpp`**: Reads the input string as an `istream` and provides low-level character navigation using `peek()` and `advance()`.
* **`tokenizer.h / tokenizer.cpp`**: Groups characters into logical **Tokens** (Numbers, Names, Operators) and filters out whitespaces.

### 🗄️ Symbol Table

* **`symbol_table.h / symbol_table.cpp`**: Manages variable storage. It maps variable names (like `x`, `y`, `z`) to specific memory addresses, allowing the VM to efficiently load values during execution.

### 🌳 Abstract Syntax Tree (AST)

Defines the hierarchical structure of the parsed expression.

* **`ast.h`**:
* `enum class OpCode`: Instruction set for the Virtual Machine (`ADD`, `SUB`, `MUL`, `DIV`, `UNARY`, `LOAD_CONST`, `LOAD_VAR`).
* `struct Instruction`: The core bytecode unit containing an `OpCode`, operand addresses, and destination index.


* **`class ASTNode`**: The abstract base class providing two vital interfaces:
* `print()`: Recursively displays the AST structure with visual markers (`|-->`) for debugging.
* `transform()`: A recursive method (Post-order traversal) that transforms the hierarchical tree into a linear vector of `Instruction` objects.


* **Concrete Nodes**: `NumberNode`, `VariableNode`, `BinaryOpNode`, `UnaryOpNode`.

### ⚙️ Parser

* **`parser.h / parser.cpp`**: Converts tokens into a structured **AST**. It implements operator precedence and handles parentheses to ensure correct mathematical evaluation order.

### 💻 Execution Engine

* **`calc.h / calc.cpp`**:
* `debug_mode`: Enables `visualize()` and `root->print()` methods. It's turned off by default.
* `compile()` : From expression creates AST. After that, it transforms that AST based on `Instruction`.
* `execute()`: Runs the bytecode using a linear memory buffer (`std::vector<double>`), making it significantly faster than direct tree evaluation.
---

## 🚀 Execution Pipeline

1. **Source Code** (String) ➡️ **Lexer/Tokenizer** (Tokens)
2. **Tokens** ➡️ **Parser** (AST Construction)
3. **AST Tree** ➡️ **Transform** (Bytecode Generation)
4. **Bytecode** ➡️ **VM Execution** (Final Result)

---

## 📊 Visual Output Examples

### 🌳 AST Tree View 
```text
    std::string expression = "(x*x + y*y + z*z) * (-0.5 + x*y / 100)";
    
    └── BinaryOp: *
    ├── BinaryOp: +
    │   ├── BinaryOp: +
    │   │   ├── BinaryOp: *
    │   │   │   ├── Var (Addr: 0)
    │   │   │   └── Var (Addr: 0)
    │   │   └── BinaryOp: *
    │   │       ├── Var (Addr: 1)
    │   │       └── Var (Addr: 1)
    │   └── BinaryOp: *
    │       ├── Var (Addr: 2)
    │       └── Var (Addr: 2)
    └── BinaryOp: +
        ├── Number: -0.5
        └── BinaryOp: /
            ├── BinaryOp: *
            │   ├── Var (Addr: 0)
            │   └── Var (Addr: 1)
            └── Number: 100
```

---
### 📑 Disassembly View

```text
[SYMBOLIC Visualization]
Addr  OpCode      L     R     Dst   Value
---------------------------------------------
[0]  LOAD_VAR    0     -     0     *var
[1]  LOAD_VAR    0     -     1     *var
[2]  MUL         0     1     2     ?
[3]  LOAD_VAR    1     -     3     *var
[4]  LOAD_VAR    1     -     4     *var
[5]  MUL         3     4     5     ?
[6]  ADD         2     5     6     ?
[7]  LOAD_VAR    2     -     7     *var
[8]  LOAD_VAR    2     -     8     *var
[9]  MUL         7     8     9     ?
[10]  ADD         6     9     10    ?
[11]  LOAD_CONST  -     -     11    -0.5
[12]  LOAD_VAR    0     -     12    *var
[13]  LOAD_VAR    1     -     13    *var
[14]  MUL         12    13    14    ?
[15]  LOAD_CONST  -     -     15    100
[16]  DIV         14    15    16    ?
[17]  ADD         11    16    17    ?
[18]  MUL         10    17    18    ?
---------------------------------------------
```

---
## 🛠️ Build and Run

To compile and run the project, use the following commands depending on your compiler:

### **Using g++ (Linux / MinGW)**

```bash
g++ -O3 *.cpp -static -o out.exe && ./out.exe
```
#### **g++ Flag Definitions:**
* `-O3` ➡️ Enables the highest level of optimization for speed. The compiler performs aggressive code              transformations to make the VM run as fast as possible. 
* `*.cpp` ➡️ Compiles all C++ source files in the current directory.
* `-static` ➡️ Links libraries statically. This ensures the `.exe` contains all necessary dependencies and can run on other machines without requiring additional DLLs or libraries.
* `-o out.exe` ➡️ Specifies the output executable file name.

### **Using MSVC (Windows - Developer Command Prompt)**

```bash
cl /EHsc /O2 /W4 *.cpp /Fe:out.exe && out.exe
```

#### **MSVC Flag Definitions:**

* `/EHsc` ➡️ Enables standard C++ stack unwinding (Exception Handling).
* `/O2` ➡️ Creates fast code (Maximum Optimization).
* `/W4` ➡️ Displays all relevant warnings for code quality.
* `*.cpp` ➡️ Compiles all C++ source files in the directory.
* `/Fe:out.exe` ➡️ Specifies the output executable file name.
---

## 🛠️ Usage Example (main.cpp)

```cpp
SymbolTable st;
st.setVariable("x", 1.0);
st.setVariable("y", 2.0);
st.setVariable("z", 3.0);

Calculate calc; // This won't call root -> print() and calc.visualize() methods
// Calculate calc(true); // This will print parse's output (The AST) and visualize its transformation (calc.visualize());

std::string expr = "(x*x + y*y + z*z) * (-0.5 + x*y / 100)";

calc.compile(expr, st);
double result = calc.execute(st);
std::cout<< "Result: " <<result<<std::endl;
return 0;
```

