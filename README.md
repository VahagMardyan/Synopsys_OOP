# 🧮 Math Expression Compiler & VM

A high-performance C++ mathematical expression evaluator that follows a full compiler pipeline: from lexical analysis to Abstract Syntax Tree (AST) construction, and finally to bytecode execution via a Register-based Virtual Machine.

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
    (x*x + y*y + z*z) * (-0.5 + x*y / 100)
--> BinaryOp: *
    |--> BinaryOp: +
        ||--> BinaryOp: +
            |||--> BinaryOp: *
                ||||--> Variable (Address: 0)
                ||||--> Variable (Address: 0)
            |||--> BinaryOp: *
                ||||--> Variable (Address: 1)
                ||||--> Variable (Address: 1)
        ||--> BinaryOp: *
            |||--> Variable (Address: 2)
            |||--> Variable (Address: 2)
    |--> BinaryOp: +
        ||--> Number: -0.5
        ||--> BinaryOp: /
            |||--> BinaryOp: *
                ||||--> Variable (Address: 0)
                ||||--> Variable (Address: 1)
            |||--> Number: 100
```

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

## 🛠️ Usage Example (main.cpp)

```cpp
SymbolTable st;
size_t x_addr = st.getAddress("x");
size_t y_addr = st.getAddress("y");
size_t z_addr = st.getAddress("z");
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
